/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2020 Uniot <info.uniot@gmail.com>
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

#pragma once

#include <libminilisp.h>
#include <TaskScheduler.h>
#include <LimitedQueue.h>
#include <Publisher.h>
#include <Bytes.h>

using namespace uniot;
using namespace std::placeholders;

class unLisp : public Publisher<int, int>
{
public:
  enum Topic { OUTPUT_BUF = 53942 };
  enum Msg { ADDED, REPLACED };

  unLisp(unLisp const &) = delete;
  void operator=(unLisp const &) = delete;

  static unLisp &getInstance()
  {
    static unLisp instance;
    return instance;
  }

  TaskScheduler::TaskPtr getTask() {
    return mTaskLispEval;
  }

  bool isCreated()
  {
    return lisp_is_created();
  }

  bool taskIsRunning()
  {
    return mTaskLispEval->isAtached();
  }

  size_t memoryUsed() {
    return lisp_mem_used();
  }

  void runCode(const Bytes& data) {
    mLastCode = data;

    mTaskLispEval->detach();
    _destroyMachine();
    _createMachine();
    
    lisp_eval(mLispRoot, mLispEnv, mLastCode.terminate().c_str());
    if (!mTaskLispEval->isAtached())
      _destroyMachine();
  }

  unLisp *pushPrimitive(const String &name, Primitive *primitive)
  {
    mUserPrimitives.push(std::make_pair(name, primitive));
    return this;
  }

  String popOutput() {
    return mOutputBuffer.popLimited("");
  }

  size_t sizeOutput() {
    return mOutputBuffer.size();
  }

private:
  unLisp()
  {
    auto fnPrint = [] (const char *msg, int size) {
      getInstance()._printToBuf(msg, size);
    };

    lisp_set_cycle_yield(yield);
    lisp_set_printers(fnPrint, fnPrint);

    _constructLispEnv();

    mOutputBuffer.limit(10);

    mTaskLispEval = TaskScheduler::make([this](short t) {
      void *root = mLispRoot;
      Obj **env = mLispEnv;

      DEFINE2(t_obj, t_pass);
      *t_pass = get_variable(root, env, "#t_pass");
      (*t_pass)->cdr->value = t;
      *t_obj = get_variable(root, env, "#t_obj")->cdr;
      safe_eval(root, env, t_obj);

      if (!t) {
        _destroyMachine();
      }
    });
  }

  void _constructLispEnv()
  {
    mLispEnvConstructor[0] = NULL;
    mLispEnvConstructor[1] = NULL;
    mLispEnvConstructor[2] = ROOT_END;
    mLispRoot = mLispEnvConstructor;
    mLispEnv = (Obj **)(mLispEnvConstructor + 1);
  }

  void _createMachine()
  {
    lisp_create(4000);

    *mLispEnv = make_env(mLispRoot, &Nil, &Nil);
    define_constants(mLispRoot, mLispEnv);
    define_primitives(mLispRoot, mLispEnv);

    add_constant(mLispRoot, mLispEnv, "#t_obj", &Nil);
    add_constant_int(mLispRoot, mLispEnv, "#t_pass", 0);
    add_primitive(mLispRoot, mLispEnv, "task", mPrimitiveTask);

    mUserPrimitives.forEach([this](std::pair<String, Primitive *> holder) {
      add_primitive(mLispRoot, mLispEnv, holder.first.c_str(), holder.second);
    });
  }

  void _destroyMachine()
  {
    lisp_destroy();
  }

  inline struct Obj *_primTask(void *root, struct Obj **env, struct Obj **list)
  {
    Obj *args = eval_list(root, env, list);
    if (length(args) != 3)
      error("Malformed task");

    Obj *times = args->car;
    Obj *ms = args->cdr->car;
    Obj *obj = args->cdr->cdr->car;

    // TODO: check types

    DEFINE1(t_obj);
    *t_obj = get_variable(root, env, "#t_obj");
    (*t_obj)->cdr = obj;

    mTaskLispEval->attach(ms->value, times->value);
    return True;
  }

  inline void _printToBuf(const char *msg, int size) {
    if(size > 0) {
      bool alreadyFull = mOutputBuffer.isFull();
      mOutputBuffer.pushLimited(String(msg));
      publish(Topic::OUTPUT_BUF, alreadyFull ? Msg::REPLACED : Msg::ADDED);
    }
    yield();
  }

  Bytes mLastCode;
  LimitedQueue<String> mOutputBuffer;
  TaskScheduler::TaskPtr mTaskLispEval;
  ClearQueue<std::pair<String, Primitive*>> mUserPrimitives;

  const Primitive *mPrimitiveTask = [](void *root, struct Obj **env, struct Obj **list) { return getInstance()._primTask(root, env, list); };

  void *mLispEnvConstructor[3];
  void *mLispRoot;
  Obj **mLispEnv;
};
