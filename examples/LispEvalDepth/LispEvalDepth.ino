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
 * Measures how much stack one level of Lisp eval nesting consumes, so that a
 * recursion limit can be chosen from measurements rather than guessed. Supports the
 * ESP8266 continuation stack and the ESP32 Arduino loop task stack.
 *
 * The measurement samples the stack pointer directly. The high water mark reported by
 * ESP.getFreeContStack() and uxTaskGetStackHighWaterMark() is deliberately not used
 * for it: those cover the whole run, so shallow probes hide under whatever peak WiFi
 * or MQTT established earlier and appear to cost nothing. The high water figure is
 * still printed, clearly labelled, for reference.
 *
 * The probe runs from a scheduler task, the context user scripts are evaluated in, so
 * the reported headroom reflects the stack already spent before eval is reached.
 *
 * Flash it and open the serial monitor at 115200. The run repeats, so attaching late
 * still catches a complete table.
 */

#include <Uniot.h>

#if defined(ESP8266)
#include <cont.h>
#elif defined(ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

using namespace uniot;

// Lowest address the running task's stack may reach, and its total size. Both chips
// grow the stack down towards the bottom returned here.
#if defined(ESP8266)
static inline uintptr_t stackBottomAddr() { return (uintptr_t)g_pcont->stack; }
static inline uint32_t stackTotalBytes() { return (uint32_t)CONT_STACKSIZE; }
static inline uint32_t stackHighWaterFree() { return ESP.getFreeContStack(); }
#elif defined(ESP32)
static inline uintptr_t stackBottomAddr() { return (uintptr_t)pxTaskGetStackStart(NULL); }
static inline uint32_t stackTotalBytes() { return (uint32_t)CONFIG_ARDUINO_LOOP_STACK_SIZE; }
static inline uint32_t stackHighWaterFree() { return (uint32_t)uxTaskGetStackHighWaterMark(NULL); }
#endif

/** Stop probing while at least this much stack is still below the deepest frame. */
static const uint32_t kStackFloor = 512;
/** Lisp recursion levels are probed in steps of this size. */
static const int kStep = 2;
/** Never probe deeper than this, whatever the measurements suggest. */
static const int kMaxLispDepth = 300;

static volatile uintptr_t sSpAtProbe = 0;
static volatile uint32_t sHighWaterFree = 0;
static volatile int sEvalDepthAtProbe = 0;

/** Stack still available below the current frame, in bytes. */
static inline uint32_t stackBelow(uintptr_t sp) {
  const uintptr_t bottom = stackBottomAddr();
  return sp > bottom ? (uint32_t)(sp - bottom) : 0;
}

/** (stack) -> bytes still free below this frame, sampled at the point of the call. */
Object stackProbe(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe("stack", Lisp::Int, 0).init(root, env, list);
  expeditor.assertDescribedArgs();

  char marker;
  sSpAtProbe = (uintptr_t)&marker;
  sHighWaterFree = stackHighWaterFree();
  sEvalDepthAtProbe = eval_depth;

  return expeditor.makeInt((int)stackBelow(sSpAtProbe));
}

/** Recurses `lispDepth` levels and samples the stack at the bottom. */
static bool probe(int lispDepth, uintptr_t &sp, int &evalNesting, uint32_t &highWater) {
  String code = F("(defun f (n) (if (= n 0) (stack) (f (+ n -1)))) (f ");
  code += lispDepth;
  code += ')';

  sSpAtProbe = 0;
  sEvalDepthAtProbe = 0;
  Uniot.getAppKit().getLisp().runCode(Bytes(code));

  sp = sSpAtProbe;
  evalNesting = sEvalDepthAtProbe;
  highWater = sHighWaterFree;
  return sp != 0;
}

static void measure() {
  char here;
  const uintptr_t spTask = (uintptr_t)&here;

  // The limit is fixed when runCode() recreates the machine, so it cannot be lifted
  // from here. The ramp stops short of it on purpose, and the suggestion below comes
  // from the measured slope rather than from reaching the ceiling.
  const int configuredLimit = UNIOT_LISP_MAX_EVAL_DEPTH;

  Serial.println();
  Serial.println(F("=== lisp eval depth probe ==="));
  Serial.printf("task stack              : %u bytes\n", (unsigned)stackTotalBytes());
  Serial.printf("free below task frame   : %u bytes  (instantaneous)\n", (unsigned)stackBelow(spTask));
  Serial.printf("high water free         : %u bytes  (peak over the whole run)\n",
                (unsigned)stackHighWaterFree());
  Serial.println();
  Serial.println(F("lisp depth  eval nesting  free below  used  bytes/eval"));

  uintptr_t spBase = 0;
  int nBase = 0;
  uint32_t hwBase = 0;
  if (!probe(0, spBase, nBase, hwBase)) {
    Serial.println(F("probe failed at depth 0"));
    return;
  }
  Serial.printf("%10d  %12d  %10u  %4s  %10s\n", 0, nBase, (unsigned)stackBelow(spBase), "-", "-");

  uint32_t bytesPerEvalX10 = 0;
  int deepestLisp = 0;
  int deepestNesting = nBase;

  for (int depth = kStep; depth <= kMaxLispDepth; depth += kStep) {
    uintptr_t sp = 0;
    int nesting = 0;
    uint32_t hw = 0;
    if (!probe(depth, sp, nesting, hw)) {
      Serial.printf("probe failed at depth %d\n", depth);
      break;
    }
    if (sp >= spBase || nesting <= nBase) {
      Serial.printf("implausible sample at depth %d, stopping\n", depth);
      break;
    }

    const uint32_t used = (uint32_t)(spBase - sp);
    const uint32_t evalLevels = (uint32_t)(nesting - nBase);
    bytesPerEvalX10 = (used * 10u) / evalLevels;
    deepestLisp = depth;
    deepestNesting = nesting;

    Serial.printf("%10d  %12d  %10u  %4u  %7u.%u\n",
                  depth, nesting, (unsigned)stackBelow(sp), (unsigned)used,
                  (unsigned)(bytesPerEvalX10 / 10u), (unsigned)(bytesPerEvalX10 % 10u));

    // Stop before one more step could cross the floor.
    const uint32_t stepCost = (used * (uint32_t)kStep) / (uint32_t)depth;
    if (stackBelow(sp) < kStackFloor + stepCost) {
      Serial.println(F("floor reached, stopping"));
      break;
    }
    // Or before it would trip the configured recursion limit, which would surface as
    // a lisp error rather than a measurement.
    const int nestingPerStep = ((nesting - nBase) * kStep) / depth;
    if (configuredLimit > 0 && nesting + nestingPerStep > configuredLimit) {
      Serial.printf("configured limit (%d) reached, stopping\n", configuredLimit);
      break;
    }
  }

  if (bytesPerEvalX10 == 0) {
    Serial.println(F("not enough samples to draw a conclusion"));
    return;
  }

  const uint32_t headroom = stackBelow(spBase);
  const uint32_t usable = headroom > kStackFloor ? headroom - kStackFloor : 0;
  const long maxEvalDepth = usable > 0 ? (long)((usable * 10u) / bytesPerEvalX10) + nBase : 0;

  Serial.println();
  Serial.printf("bytes per eval level    : %u.%u\n",
                (unsigned)(bytesPerEvalX10 / 10u), (unsigned)(bytesPerEvalX10 % 10u));
  Serial.printf("eval nesting at depth 0 : %d\n", nBase);
  Serial.printf("free below depth 0      : %u bytes\n", (unsigned)headroom);
  Serial.printf("reserved floor          : %u bytes\n", (unsigned)kStackFloor);
  Serial.printf("deepest probed          : lisp %d / eval nesting %d\n", deepestLisp, deepestNesting);
  Serial.println();
  Serial.printf("suggested MAX_EVAL_DEPTH: %ld\n", maxEvalDepth);
  Serial.printf("currently configured    : %d\n", configuredLimit);
  Serial.println(F("(compare against eval_depth, which counts entries to eval)"));
}

auto taskMeasure = Uniot.createTask("measure", [](SchedulerTask &self, short t) {
  measure();
});

void setup() {
  Serial.begin(115200);
  delay(200);

  Uniot.addLispPrimitive(stackProbe);
  Uniot.begin();

  // Repeats so that a host attaching late still catches a complete run, which
  // matters on the ESP32C3 where the USB serial port re-enumerates on reset.
  taskMeasure->attach(20000, 0);
}

void loop() {
  Uniot.loop();
}
