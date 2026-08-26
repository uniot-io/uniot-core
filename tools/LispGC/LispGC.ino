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


// Sized to fit a 7000 byte heap and the stack budget the firmware runs with.
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

  // Not in tail position, so every level is a real frame. Two is what the firmware's
  // stack budget holds for this shape.
  {"recursion", "2", false,
   "(defun d (n) (if (= n 0) 0 (+ 1 (d (+ n -1)))))(d 2)"},

  {"mixed-sizes", "0", true,
   "(define a ())(define b ())(define i 0)"
   "(while (< i 200) (setq a (list i)) (setq b (list i i i i i i i i)) (setq a ()) (setq i (+ i 1)))"
   "(setq b ())0"},

  // Five hundred nested calls would need roughly 92000 bytes of stack at 184 bytes a
  // level, against a budget of 2048. It only fits because a call in tail position costs
  // nothing, so this fails outright if tail call elimination stops working on target.
  {"tail-calls", "0", true,
   "(defun lp (n) (if (= n 0) 0 (lp (+ n -1))))(lp 500)"},

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
  Serial.println(F("  workload      total_us    gc_us  pause_us  colls  allocs  peak_live  peak_sys  blocks  largest"));

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
    Serial.printf("  %-12s %8lu %8lu %9lu %6u %7u %10u %9u %7u %8u%s\n",
                  w->name,
                  (unsigned long)best,
                  (unsigned long)snapshot.gc_time,
                  (unsigned long)snapshot.gc_time_max,
                  (unsigned)snapshot.collections,
                  (unsigned)snapshot.allocations,
                  (unsigned)snapshot.live_peak,
                  (unsigned)snapshot.system_peak,
                  (unsigned)snapshot.free_blocks,
                  (unsigned)snapshot.free_largest, thin);
    Serial.flush();
  }
}

// Long-run behaviour: does the free space degrade as a script runs for a long time, or
// does it settle? A device runs the same script for weeks, so the question is whether the
// heap after ten thousand collections looks like the heap after ten.
//
// Sweep merges neighbouring free blocks on every collection, so unlike a malloc heap this
// one has no memory of its own history -- but that is an argument, and this measures it.
// What one turn of a loop costs, which is the only thing that turns
// MINILISP_MAX_LOOP_ITERATIONS from a number into a length of time. The limit exists to
// stop a loop that never ends, and the question it answers -- how long a device sits
// there before saying so -- cannot be answered on a workstation: the same iteration is
// two orders of magnitude cheaper there.
//
// Two bodies, because the spread between them is most of the answer. An empty body is
// the floor, and a body that allocates is what a real rule does.
struct LoopCase {
  const char *name;
  const char *code;
  unsigned long iterations;
};

static const LoopCase LOOP_CASES[] = {
  {"empty body", "(while (< #itr 10000) 1) 0", 10000},
  {"setq + add", "(define n 0)(while (< #itr 10000) (setq n (+ n 1))) 0", 10000},
  {"nested call", "(defun f (x) (+ x 1))(define n 0)"
                  "(while (< #itr 10000) (setq n (f n))) 0", 10000},
};

static void phase_loop_cost() {
  Serial.println();
  Serial.println(F("== loop cost: what MINILISP_MAX_LOOP_ITERATIONS is worth in seconds =="));
  Serial.println(F("  case            us/iter   10k iters    at 20000    at 100000"));

  for (size_t i = 0; i < sizeof(LOOP_CASES) / sizeof(LOOP_CASES[0]); i++) {
    const LoopCase *c = &LOOP_CASES[i];
    Workload w = {c->name, "0", false, c->code};

    // Best of three: the interest is in what an iteration costs, not in whatever else
    // the board happened to be doing during one of the runs.
    uint32_t best = 0;
    for (int r = 0; r < 3; r++) {
      uint32_t took = 0;
      if (!run(&w, &took)) {
        Serial.printf("  %-14s RAISED: %s\n", c->name, last_error);
        best = 0;
        break;
      }
      if (r == 0 || took < best)
        best = took;
    }
    if (best == 0)
      continue;

    const float per_iter = (float)best / (float)c->iterations;
    Serial.printf("  %-14s %7.2f %9lu ms %8.2f s %10.2f s\n",
                  c->name,
                  per_iter,
                  (unsigned long)(best / 1000),
                  per_iter * 20000.0f / 1000000.0f,
                  per_iter * 100000.0f / 1000000.0f);
    Serial.flush();
  }
}

static void phase_soak(unsigned long rounds) {
  Serial.println();
  Serial.println(F("== long run: free space over many collections =="));
  Serial.println(F("     round     colls    blocks   largest      live"));

  void *envConstructor[3];
  envConstructor[0] = NULL;
  envConstructor[1] = NULL;
  envConstructor[2] = ROOT_END;
  void *root = envConstructor;
  Obj **genv = (Obj **)(envConstructor + 1);

  lisp_create(HEAP, MAX_EVAL_STACK);
  if (!lisp_is_created()) {
    Serial.println(F("  no heap"));
    return;
  }
  *genv = make_env(root, &Nil, &Nil);
  define_constants(root, genv);
  define_primitives(root, genv);

  lisp_eval(root, genv,
            "(define tbl ())(define i 0)"
            "(while (< i 8) (setq tbl (cons (list i i) tbl)) (setq i (+ i 1)))"
            "(define t1 ())(define t2 ())(define t3 ())");
  lisp_stats_reset();

  // A live set that grows and shrinks around a stable core, with allocations of several
  // different sizes, so survivors land in different places each time round.
  static const char *const shapes[] = {
    "(setq tbl (cons (list 1 2 3 4 5 6) tbl))",
    "(setq t1 (list 1 2 3))",
    "(setq tbl (cdr tbl))",
    "(setq t2 (list 1 2 3 4 5 6 7 8 9 10 11 12))",
    "(setq tbl (cons (list 1) tbl))",
    "(setq t3 (list (list 1 2) (list 3 4 5)))",
    "(setq tbl (cdr tbl))",
    "(setq t1 ((lambda (x) (list x x x)) 9))",
  };
  const int nshapes = (int)(sizeof(shapes) / sizeof(shapes[0]));

  for (unsigned long r = 1; r <= rounds; r++) {
    if (!lisp_eval(root, genv, shapes[r % nshapes])) {
      Serial.printf("  raised at round %lu: %s\n", r, last_error);
      break;
    }
    if (r % (rounds / 10) == 0) {
      Serial.printf("  %9lu %9u %9u %9u %9u\n", r,
                    (unsigned)lisp_stats.collections, (unsigned)lisp_stats.free_blocks,
                    (unsigned)lisp_stats.free_largest, (unsigned)lisp_mem_used());
      Serial.flush();
    }
    yield();
  }
  lisp_destroy();
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
  phase_loop_cost();
  phase_soak(20000);

  Serial.println();
  Serial.printf("free heap after: %u bytes, largest block %u\n",
                (unsigned)ESP.getFreeHeap(), (unsigned)largestFreeBlock());
  Serial.println(F("DONE"));
}

void loop() {
  delay(1000);
}
