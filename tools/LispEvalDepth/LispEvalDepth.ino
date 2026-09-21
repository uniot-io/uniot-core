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
 * Works out how deep Lisp recursion may safely go, by measuring rather than guessing.
 * Uniot is deliberately not involved: only the interpreter is linked, so the numbers
 * describe the interpreter and nothing else.
 *
 * Three figures decide the limit, and the sketch measures all three:
 *
 *   1. What one nested call costs. Sampled from the stack pointer inside a
 *      running program, not from a high water mark: a high water mark covers the whole
 *      run, so a shallow probe hides under an earlier peak and appears to cost nothing.
 *
 *   2. What raising costs. The guard exists to stop a recursion before it runs off the
 *      end of the stack, and it reports by formatting a message -- at the deepest point,
 *      where there is least room. If that does not fit, the guard overruns the stack it
 *      is there to protect. This is measured first, while the high water mark is still
 *      clean.
 *
 *   3. How much stack there is to begin with.
 *
 * One caveat on reading the result. Linking only the interpreter leaves more stack free
 * than the firmware has, because the firmware has already spent some getting to the point
 * where a script is evaluated. Set STACK_ALREADY_SPENT below to that amount to have the
 * suggestion account for it; the cost per level is a property of the interpreter and
 * transfers unchanged either way.
 */

#include <Arduino.h>

extern "C" {
#include "libminilisp.h"
}

#if defined(ESP8266)
#include <cont.h>
extern "C" cont_t *g_pcont;
#elif defined(ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

// Lowest address the running task's stack may reach, its size, and the least it has ever
// had free. Both chips grow the stack downwards towards the bottom returned here.
#if defined(ESP8266)
static inline uintptr_t stackBottomAddr() { return (uintptr_t)g_pcont->stack; }
static inline uint32_t stackTotalBytes() { return (uint32_t)CONT_STACKSIZE; }
static inline uint32_t stackHighWaterFree() { return ESP.getFreeContStack(); }
#elif defined(ESP32)
static inline uintptr_t stackBottomAddr() { return (uintptr_t)pxTaskGetStackStart(NULL); }
static inline uint32_t stackTotalBytes() { return (uint32_t)CONFIG_ARDUINO_LOOP_STACK_SIZE; }
static inline uint32_t stackHighWaterFree() { return (uint32_t)uxTaskGetStackHighWaterMark(NULL); }
#else
#error "this sketch needs an ESP8266 or an ESP32"
#endif

// Stack the firmware has already consumed before it reaches eval. Zero here measures the
// interpreter alone; set it from the firmware to have the suggestion apply to it.
#ifndef STACK_ALREADY_SPENT
#define STACK_ALREADY_SPENT 0
#endif

// Heap for the probes. Small: they allocate almost nothing and the heap is not the
// subject here.
#ifndef PROBE_HEAP
#define PROBE_HEAP 4000
#endif

// Never leave less than this below the deepest frame, either while probing or in the
// limit that comes out at the end.
static const uint32_t kStackFloor = 256;
// Lisp recursion levels are probed in steps of this size.
static const int kStep = 1;
// Never probe deeper than this, whatever the measurements suggest.
static const int kMaxLispDepth = 400;

static int gEntryFree = 0;
static volatile uintptr_t sSpAtProbe = 0;


static char sLastResult[64];
static char sLastError[128];

static void sinkOut(const char *msg, int size) {
  (void)size;
  snprintf(sLastResult, sizeof(sLastResult), "%s", msg);
}

static void sinkErr(const char *msg, int size) {
  (void)size;
  if (sLastError[0] == '\0')
    snprintf(sLastError, sizeof(sLastError), "%s", msg);
}

/** Stack still available below the given frame, in bytes. */
static inline uint32_t stackBelow(uintptr_t sp) {
  const uintptr_t bottom = stackBottomAddr();
  return sp > bottom ? (uint32_t)(sp - bottom) : 0;
}

/** (stack) -> bytes still free below this frame, sampled at the point of the call. */
static Obj *primStack(void *root, Obj **env, Obj **list) {
  char marker;
  sSpAtProbe = (uintptr_t)&marker;
  return make_int(root, (int)stackBelow(sSpAtProbe));
}

/**
 * Evaluates one program from a clean interpreter.
 * @param stackBudget bytes of C stack the evaluation may spend, or 0 to lift the cap.
 * @return true if the program ran without raising.
 */
static bool evaluate(const char *code, size_t stackBudget) {
  void *envConstructor[3];
  envConstructor[0] = NULL;
  envConstructor[1] = NULL;
  envConstructor[2] = ROOT_END;
  void *root = envConstructor;
  Obj **genv = (Obj **)(envConstructor + 1);

  sLastResult[0] = '\0';
  sLastError[0] = '\0';
  sSpAtProbe = 0;

  lisp_create(PROBE_HEAP, stackBudget);
  if (!lisp_is_created()) {
    snprintf(sLastError, sizeof(sLastError), "no heap");
    return false;
  }

  *genv = make_env(root, &Nil, &Nil);
  define_constants(root, genv);
  define_primitives(root, genv);
  add_primitive(root, genv, "stack", primStack);

  gEntryFree = (int)stackBelow((uintptr_t)&envConstructor);
  const bool ok = lisp_eval(root, genv, code);
  lisp_destroy();
  return ok;
}

/** Recurses `lispDepth` calls deep and samples the stack at the bottom, with no cap. */
static bool probe(int lispDepth, uintptr_t &sp) {
  char code[160];
  snprintf(code, sizeof(code),
           // Wrapped in an addition on purpose: the call must NOT be in tail position,
           // or the interpreter eliminates it and the probe measures a stack that never
           // grows. What is being measured here is the cost of a real nested call.
           "(defun f (n) (if (= n 0) (stack) (+ 0 (f (+ n -1)))))(f %d)", lispDepth);
  if (!evaluate(code, 0))
    return false;
  sp = sSpAtProbe;
  return sp != 0;
}

/**
 * What raising costs: one eval frame of overshoot plus the whole error path, measured
 * from the stack where evaluation began down to the deepest point reached.
 *
 * A budget of one byte makes the guard fire on the first eval it checks, so nothing is
 * mixed in from the cost of getting deep. It has to run before any other probe, because
 * the high water mark only ever falls -- once a deep probe has moved it, this can no
 * longer see its own excursion.
 *
 * @return bytes that must stay in reserve below the limit, or 0 if it could not be
 *         measured.
 */
static uint32_t measureRaiseCost() {
  if (evaluate("(defun r (n) (+ 1 (r (+ n 1))))(r 0)", 1))
    return 0;  // it was supposed to raise
  const uint32_t deepest = stackHighWaterFree();
  const uint32_t entry = (uint32_t)gEntryFree;
  return entry > deepest ? entry - deepest : 0;
}

static void measure() {
  Serial.println();
  Serial.println(F("=== lisp eval depth ==="));
  Serial.printf("task stack            : %u bytes\n", (unsigned)stackTotalBytes());

  const uint32_t raiseCost = measureRaiseCost();
  if (raiseCost)
    Serial.printf("raising an error costs: %u bytes\n", (unsigned)raiseCost);
  else
    Serial.println(F("raising an error costs: could not measure"));

  uintptr_t spBase = 0;
  if (!probe(0, spBase)) {
    Serial.println(F("probe failed at depth 0"));
    return;
  }
  const uint32_t headroom = stackBelow(spBase);
  Serial.printf("free below depth 0    : %u bytes\n", (unsigned)headroom);
  Serial.println();
  Serial.println(F("     calls  free below  used  bytes/call"));
  Serial.printf("%10d  %10u  %4s  %10s\n", 0, (unsigned)headroom, "-", "-");

  uint32_t perLevel = 0;
  int deepestLisp = 0;

  for (int depth = kStep; depth <= kMaxLispDepth; depth += kStep) {
    uintptr_t sp = 0;
    if (!probe(depth, sp)) {
      Serial.printf("probe stopped at depth %d: %s\n", depth, sLastError);
      break;
    }
    if (sp >= spBase) {
      Serial.printf("implausible sample at depth %d, stopping\n", depth);
      break;
    }

    const uint32_t used = (uint32_t)(spBase - sp);
    perLevel = used / (uint32_t)depth;
    deepestLisp = depth;

    Serial.printf("%10d  %10u  %4u  %10u\n",
                  depth, (unsigned)stackBelow(sp), (unsigned)used, (unsigned)perLevel);
    Serial.flush();

    // Stop before one more step could cross the floor, counting the room raising needs.
    const uint32_t stepCost = (used * (uint32_t)kStep) / (uint32_t)depth;
    if (stackBelow(sp) < kStackFloor + raiseCost + stepCost) {
      Serial.println(F("floor reached, stopping"));
      break;
    }
  }

  if (perLevel == 0) {
    Serial.println(F("not enough samples to draw a conclusion"));
    return;
  }

  const uint32_t reserved = raiseCost + kStackFloor + (uint32_t)STACK_ALREADY_SPENT;
  const uint32_t budget = headroom > reserved ? headroom - reserved : 0;

  Serial.println();
  Serial.printf("bytes per nested call   : %u\n", (unsigned)perLevel);
  Serial.printf("deepest probed          : %d calls\n", deepestLisp);
  Serial.printf("free below depth 0      : %u bytes\n", (unsigned)headroom);
  Serial.printf("reserved for raising    : %u bytes\n", (unsigned)raiseCost);
  Serial.printf("reserved as floor       : %u bytes\n", (unsigned)kStackFloor);
  Serial.printf("assumed already spent   : %u bytes\n", (unsigned)STACK_ALREADY_SPENT);
  Serial.println();
  Serial.printf("=> UNIOT_LISP_MAX_EVAL_STACK : %u bytes\n", (unsigned)budget);
  Serial.printf("   which is about %u nested calls of the shape probed here\n",
                (unsigned)(budget / perLevel));
  Serial.println(F("   If STACK_ALREADY_SPENT is 0 this is for the interpreter alone"));
  Serial.println(F("   and is optimistic for the firmware."));
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  lisp_set_cycle_yield(yield);
  lisp_set_printers(sinkOut, NULL, sinkErr);

  measure();

  Serial.println(F("DONE"));
}

void loop() {
  delay(1000);
}
