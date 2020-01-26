#ifndef _SKETCH_SELECTED_
#define _SKETCH_SELECTED_

#include <Board-WittyCloud.h>
#include <Arduino.h>
#include <unLisp.h>

TaskScheduler::TaskPtr task;

struct Obj *user_prim_led(void *root, struct Obj **env, struct Obj **list)
{
  auto args = eval_list(root, env, list);
  if (length(args) != 2)
    error("Malformed task");

  auto objColor = args->car;
  auto objState = args->cdr->car;

  // TODO: check types

  auto color = objColor->value;
  auto state = objState != Nil;
  if (objState->type == TINT) {
    state = objState->value != 0;
  }

  uint8_t pin = 0;
  switch (color) {
    case 0:
      pin = RED;
      break;
    case 1:
      pin = GREEN;
      break;
    case 2:
      pin = BLUE;
      break;
    default:
      error("Out of range: color: [0, 1, 2]");
  }

  digitalWrite(pin, state);
  return objState;
}

struct Obj *user_prim_ldr(void *root, struct Obj **env, struct Obj **list)
{
  DEFINE1(objLdr);
  *objLdr = make_int(root, analogRead(LDR));
  return *objLdr;
}

void setup()
{
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(LDR, INPUT);

  Serial.begin(9600);
  Serial.println();
  Serial.println();

  // unLisp::getInstance().runCode("(setq a 1) (print #pass) (print #t) (setq #pass 1)");
  // unLisp::getInstance().runCode("(print #itr) (while (< #itr 200) (print #itr)) (print #itr)");
  // unLisp::getInstance().runCode("(define code '(+ 1 2)) (eval '(+ 2 2)) (eval code) (print code) (+ 5 6)");
  // unLisp::getInstance().runCode("(task 10 100 '(+ 100 #t_pass))");

  unLisp::getInstance().pushPrimitive("led", user_prim_led);
  unLisp::getInstance().pushPrimitive("ldr", user_prim_ldr);
  task = TaskScheduler::make([](short t) {
    static bool task_switch = false;
    Serial.print("- Before: ESP mem left:");
    Serial.println(ESP.getFreeHeap());
    if (task_switch = !task_switch)
      unLisp::getInstance().runCode("(defun color (n) (% n 3))(task 0 500 '(list (print (ldr)) (led 0 0) (led 1 0) (led 2 0) (led ( - (/ (ldr) 100) 1) 1)))");
    else
      unLisp::getInstance().runCode("(defun odd (n) (= 1 (% n 2))) (defun color (n) (% n 3)) (task 30 200 '(led (color #t_pass) (odd #t_pass)))");
    Serial.print("-After: ESP mem left:");
    Serial.println(ESP.getFreeHeap());
  });
  task->attach(10000);
}

void loop()
{
  unLisp::getInstance().getTask()->execute();
  task->execute();
}

#endif // _SKETCH_SELECTED_
