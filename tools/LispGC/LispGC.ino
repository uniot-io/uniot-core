/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2026 Uniot <contact@uniot.io>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Exercises the Lisp garbage collector on real hardware, with nothing else running.
 * Uniot is deliberately not involved: this measures the interpreter, and WiFi, MQTT and
 * the scheduler would only add noise to the timings and pressure to the heap.
 *
 * Two phases. The first checks that every collection leaves a sound heap, by collecting
 * before every allocation and walking the whole heap afterwards. The second turns those
 * checks off and measures what collecting actually costs.
 *
 * The strategy is chosen when the library is compiled, so there is one environment per
 * strategy per board. Flash each in turn and compare:
 *
 *   pio run -e esp12e-copying -t upload    && pio device monitor
 *   pio run -e esp12e-mark-sweep -t upload && pio device monitor
 */

#include <Arduino.h>

extern "C" {
#include "libminilisp.h"
}

#if MINILISP_GC == MINILISP_GC_COPYING
#define STRATEGY "copying"
#else
#define STRATEGY "mark-sweep"
#endif

// Declared before any function, because the Arduino preprocessor inserts generated
// prototypes ahead of the first definition and they have to be able to name this.
struct Workload {
  const char *name;
  const char *expect;
  // Whether this one is here to exercise the collector. `recursion` is not -- it measures
  // what nested calls cost and never fills the heap -- so it is not expected to collect
  // and is not complained about for failing to.
  bool collects;
  const char *code;
};

// The largest single allocation the system would grant right now. This is the figure a
// copying collector lives or dies by, since it has to obtain a second heap in one piece.
static size_t largestFreeBlock() {
#if defined(ESP8266)
  return ESP.getMaxFreeBlockSize();
#elif defined(ESP32)
  return ESP.getMaxAllocHeap();
#else
  return 0;
#endif
}

// The firmware's own settings, so that the numbers describe what actually ships.
// Overridable from platformio.ini.
#ifndef UNIOT_LISP_HEAP
#define UNIOT_LISP_HEAP 7000
#endif
#if defined(ESP8266)
#define DEFAULT_MAX_EVAL_STACK 2048
#else
#define DEFAULT_MAX_EVAL_STACK 5632
#endif
#ifndef UNIOT_LISP_MAX_EVAL_STACK
#define UNIOT_LISP_MAX_EVAL_STACK DEFAULT_MAX_EVAL_STACK
#endif

static const size_t HEAP = UNIOT_LISP_HEAP;
// Bytes of C stack one evaluation may spend, which is what the interpreter's recursion
// guard actually measures.
static const size_t MAX_EVAL_STACK = UNIOT_LISP_MAX_EVAL_STACK;

static char last_result[64];
static char last_error[96];

static void out_handler(const char *msg, int size) {
  (void)size;
  snprintf(last_result, sizeof(last_result), "%s", msg);
}

static void err_handler(const char *msg, int size) {
  (void)size;
  if (last_error[0] == '\0')
    snprintf(last_error, sizeof(last_error), "%s", msg);
}

// The same two hooks the REPL adds, so a program can force a collection and ask how
// much is live.
static Obj *prim_mem(void *root, Obj **env, Obj **list) {
  return make_int(root, (int)lisp_mem_used());
}

static Obj *prim_gc(void *root, Obj **env, Obj **list) {
  gc(root);
  return Nil;
}


// Sized to fit a 7000 byte heap and, more restrictively, an eval nesting limit of 16.
static const Workload WORKLOADS[] = {
  {"churn", "0", true,
   "(define i 0)(define j ())"
   "(while (< i 350) (setq j (list 1 2 3 (list 4 5) 6)) (setq i (+ i 1)))"
   "(setq j ())0"},

  {"retain", "0", true,
   "(define n ())(define i 0)"
   "(while (< i 40) (setq n (cons (list i i) n)) (setq i (+ i 1)))"
   "(gc)(gc)(gc)0"},

  {"branching", "59", true,
   "(define n ())(define i 0)"
   "(while (< i 60) (setq n (cons (list i) n)) (setq i (+ i 1)))"
   "(gc)(gc)(gc)(car (car n))"},

  {"working-set", "0", true,
   "(define tbl ())(define i 0)"
   "(while (< i 20) (setq tbl (cons (list i (* i 2)) tbl)) (setq i (+ i 1)))"
   "(define t ())(setq i 0)"
   "(while (< i 250) (setq t (list i (list i i) i)) (setq i (+ i 1)))"
   "(setq t ())0"},

  {"closures", "7", true,
   "(define i 0)(define f ())"
   "(while (< i 250) (setq f ((lambda (x) (lambda (y) (+ x y))) i)) (setq i (+ i 1)))"
   "(setq f ())(+ 3 4)"},

  // Depth 16 is the whole nesting budget on this board, and each level of `d` costs
  // several. Two is what fits; five overruns the continuation stack before the guard
  // can raise.
  {"recursion", "2", false,
   "(defun d (n) (if (= n 0) 0 (+ 1 (d (+ n -1)))))(d 2)"},

  {"mixed-sizes", "0", true,
   "(define a ())(define b ())(define i 0)"
   "(while (< i 200) (setq a (list i)) (setq b (list i i i i i i i i)) (setq a ()) (setq i (+ i 1)))"
   "(setq b ())0"},

  {"survives", "(1 2 3)", true,
   "(define keep (list 1 2 3))(gc)(gc)(gc) keep"},
};

static const size_t WORKLOAD_COUNT = sizeof(WORKLOADS) / sizeof(WORKLOADS[0]);

static bool run(const Workload *w, uint32_t *micros_taken) {
  void *env_constructor[3];
  env_constructor[0] = NULL;
  env_constructor[1] = NULL;
  env_constructor[2] = ROOT_END;
  void *root = env_constructor;
  Obj **genv = (Obj **)(env_constructor + 1);

  last_result[0] = '\0';
  last_error[0] = '\0';

  lisp_create(HEAP, MAX_EVAL_STACK);
  if (!lisp_is_created()) {
    snprintf(last_error, sizeof(last_error), "no heap");
    return false;
  }

  *genv = make_env(root, &Nil, &Nil);
  define_constants(root, genv);
  define_primitives(root, genv);
  add_primitive(root, genv, "mem", prim_mem);
  add_primitive(root, genv, "gc", prim_gc);

  lisp_stats_reset();

  const uint32_t started = micros();
  const bool ok = lisp_eval(root, genv, w->code);
  *micros_taken = micros() - started;

  lisp_destroy();
  return ok;
}

static uint64_t clock_us() { return (uint64_t)micros(); }

static void phase_correctness() {
  Serial.println();
  Serial.println(F("== correctness: heap verified after every collection, collect before every allocation =="));

  // The heavy settings. Every allocation collects, and every collection is followed by a
  // walk of the whole heap that halts the board if an invariant is broken.
  always_gc = true;
  verify_gc = true;

  int failures = 0;
  for (size_t i = 0; i < WORKLOAD_COUNT; i++) {
    const Workload *w = &WORKLOADS[i];
    Serial.printf("  %-12s ", w->name);
    Serial.flush();

    uint32_t took = 0;
    const bool ok = run(w, &took);
    if (!ok) {
      Serial.printf("RAISED: %s\n", last_error);
      failures++;
    } else if (strcmp(last_result, w->expect) != 0) {
      Serial.printf("WRONG: expected %s, got %s\n", w->expect, last_result);
      failures++;
    } else {
      Serial.printf("ok  (%lu ms)\n", (unsigned long)(took / 1000));
    }
    Serial.flush();
  }

  always_gc = false;
  verify_gc = false;
  Serial.printf("  %d failure(s)\n", failures);
}

static void phase_measurement() {
  Serial.println();
  Serial.println(F("== cost: normal settings =="));
  Serial.println(F("  workload      total_us    gc_us  pause_us  colls  allocs  peak_live  peak_sys  blocks  largest  depth"));

  for (size_t i = 0; i < WORKLOAD_COUNT; i++) {
    const Workload *w = &WORKLOADS[i];

    uint32_t best = 0;
    LispStats snapshot;
    for (int r = 0; r < 3; r++) {
      uint32_t took = 0;
      if (!run(w, &took)) {
        Serial.printf("  %-12s RAISED: %s\n", w->name, last_error);
        best = 0;
        break;
      }
      if (r == 0 || took < best) {
        best = took;
        snapshot = lisp_stats;
      }
    }
    if (best == 0)
      continue;

    // A workload meant to exercise the collector that has stopped doing so has stopped
    // measuring anything. That can happen quietly when objects shrink or the heap grows,
    // which is exactly when these numbers are being compared against older ones.
    const char *thin = (w->collects && snapshot.collections < 3)
                           ? "  <- too few collections, rescale this workload"
                           : "";
    Serial.printf("  %-12s %8lu %8lu %9lu %6u %7u %10u %9u %7u %8u %6d%s\n",
                  w->name,
                  (unsigned long)best,
                  (unsigned long)snapshot.gc_time,
                  (unsigned long)snapshot.gc_time_max,
                  (unsigned)snapshot.collections,
                  (unsigned)snapshot.allocations,
                  (unsigned)snapshot.live_peak,
                  (unsigned)snapshot.system_peak,
                  (unsigned)snapshot.free_blocks,
                  (unsigned)snapshot.free_largest,
                  eval_depth_max, thin);
    Serial.flush();
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  // The while loop yields to the watchdog through this.
  lisp_set_cycle_yield(yield);
  lisp_set_printers(out_handler, NULL, err_handler);
  lisp_stats_clock = clock_us;

  Serial.println();
  Serial.println(F("================================================================"));
  Serial.printf("strategy      %s\n", STRATEGY);
  Serial.printf("lisp heap     %u bytes\n", (unsigned)HEAP);
  Serial.printf("eval stack    %u bytes\n", (unsigned)MAX_EVAL_STACK);
  Serial.printf("free heap     %u bytes\n", (unsigned)ESP.getFreeHeap());
  Serial.printf("largest block %u bytes\n", (unsigned)largestFreeBlock());
  Serial.println(F("================================================================"));

  phase_correctness();
  phase_measurement();

  Serial.println();
  Serial.printf("free heap after: %u bytes, largest block %u\n",
                (unsigned)ESP.getFreeHeap(), (unsigned)largestFreeBlock());
  Serial.println(F("DONE"));
}

void loop() {
  delay(1000);
}
