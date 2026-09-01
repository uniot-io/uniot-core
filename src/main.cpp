#include <Uniot.h>

#include "primitives.h"

#if defined(ESP8266)
#define PIN_LDR A0
#define PIN_RED 15
#define PIN_GREEN 12
#define PIN_BLUE 13
#define PIN_LED 2
#define PIN_BUTTON 4
#define LED_PIN_LEVEL HIGH
#define BTN_PIN_LEVEL LOW
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
#define PIN_VIBRO 10
#define PIN_LED 8
#define PIN_BUTTON 3
#define LED_PIN_LEVEL LOW
#define BTN_PIN_LEVEL LOW
#elif defined(ESP32)
// GPIO 6-11 are the SPI flash bus on a classic ESP32 -- driving one stops the chip.
#define PIN_VIBRO 4
#define PIN_LED 2
#define PIN_BUTTON 0
#define LED_PIN_LEVEL HIGH
#define BTN_PIN_LEVEL LOW
#endif

using namespace uniot;

auto taskPrintHeap = Uniot.createTask("print_heap", [](SchedulerTask& self, short t) {
  Serial.println(ESP.getFreeHeap());
});

auto taskPrintTime = Uniot.createTask("print_time", [](SchedulerTask& self, short t) {
  Serial.println(Date::getFormattedTime());
});


// ---------------------------------------------------------------------------------
// Stack headroom probe for tuning UNIOT_LISP_MAX_EVAL_STACK. Built only with
// -D UNIOT_STACK_PROBE, so a production build carries none of it. Reports the least
// free stack the task ever had, which is what those budgets must stay under.
#ifdef UNIOT_STACK_PROBE
#include <unLisp.h>

static size_t probeFreeStack() {
#if defined(ESP32)
  // In words on FreeRTOS, and it is the minimum ever seen, not the current figure.
  return uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t);
#else
  return ESP.getFreeContStack();
#endif
}

static void probeRun(const char *name, const char *code) {
  Serial.printf("\n--- %s\n", name);
  Serial.printf("    free before      : %u bytes\n", (unsigned)probeFreeStack());
  // Reset per script, so each reports what it alone spent.
  eval_stack_max = 0;
  unLisp::getInstance().runCode(Bytes((const uint8_t *)code, strlen(code)));
  Serial.printf("    eval spent       : %u bytes (of %u budget)\n",
                (unsigned)eval_stack_max, (unsigned)UNIOT_LISP_MAX_EVAL_STACK);
  Serial.printf("    least ever free  : %u bytes\n", (unsigned)probeFreeStack());
  Serial.flush();
}

auto taskStackProbe = Uniot.createTask("stack_probe", [](SchedulerTask& self, short t) {
  Serial.println(F("\n=== lisp stack headroom, on the real firmware ==="));
  Serial.printf("    budget in force  : %u bytes\n", (unsigned)UNIOT_LISP_MAX_EVAL_STACK);
  Serial.printf("    free at rest     : %u bytes\n", (unsigned)probeFreeStack());

  // Non-tail recursion until the guard fires, then raising, which costs more on top.
  probeRun("recursion to the guard, then raising",
           "(defun d (n) (if (= n 0) 0 (+ 1 (d (- n 1))))) (d 200)");

  // The case the guard cannot see: recursion stopping just under the budget, then a
  // primitive reaching the event bus. Swept, because the worst depth is the deepest one
  // that does not itself trip first.
  for (int depth = 12; depth <= 24; depth += 4) {
    char code[160];
    snprintf(code, sizeof(code),
             "(defun d (n) (if (= n 0) (push_event 'probe 1) (progn (d (- n 1)) 0))) (d %d)",
             depth);
    char label[64];
    snprintf(label, sizeof(label), "a primitive at depth %d", depth);
    probeRun(label, code);
  }

  probeRun("the scheduled task path",
           "(defun d (n) (if (= n 0) (push_event 'probe 1) (progn (d (- n 1)) 0)))"
           "(task 3 200 '(d 10))");

  Serial.println(F("\n=== probe done ==="));
  Serial.flush();
});
#endif

void setup() {
  Serial.begin(115200);
  Uniot.configWiFiResetButton(PIN_BUTTON, BTN_PIN_LEVEL);
  Uniot.configWiFiResetOnReboot(100);

#if defined(ESP8266)
  Uniot.configWiFiStatusLed(PIN_RED, LED_PIN_LEVEL);
  Uniot.registerLispDigitalOutput(PIN_RED, PIN_GREEN, PIN_BLUE);
  Uniot.registerLispDigitalInput(0, PIN_BUTTON);
  Uniot.registerLispAnalogOutput(PIN_RED, PIN_GREEN, PIN_BLUE);
  Uniot.registerLispAnalogInput(PIN_LDR);
#elif defined(ESP32)
  Uniot.configWiFiStatusLed(PIN_LED, LED_PIN_LEVEL);
  Uniot.registerLispDigitalOutput(PIN_LED, PIN_VIBRO);
  Uniot.registerLispDigitalInput(PIN_BUTTON);
#endif

  taskPrintHeap->attach(500);
  taskPrintTime->attach(500);

  Uniot.addLispPrimitive(filter_events);
  Uniot.begin();

#ifdef UNIOT_STACK_PROBE
  // After begin(), so the machine and its primitives exist. Once, a second in, to let
  // the scheduler settle first.
  taskStackProbe->attach(1000, 1);
#endif
}

void loop() {
  Uniot.loop();
}
