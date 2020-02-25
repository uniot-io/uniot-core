/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2020 Uniot <contact@uniot.io>
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
#include <CBORArray.h>
#include <Bytes.h>
#include <Common.h>
#include <LispHelper.h>
#include <PrimitiveExpeditor.h>
#include <Logger.h>

namespace uniot
{
using namespace lisp;

class unLisp : public GeneralPublisher
{
public:
  enum Topic { LISP = FOURCC(lisp) };
  enum Msg { MSG_ADDED, MSG_REPLACED, ERROR };

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
    if (!data.size())
      return;

    mLastCode = data;
    mLastError = "";

    mTaskLispEval->detach();
    _destroyMachine();
    _createMachine();
    
    auto code = mLastCode.terminate().c_str();
    UNIOT_LOG_DEBUG("eval: %s", code);

    lisp_eval(mLispRoot, mLispEnv, code);
    if (!mTaskLispEval->isAtached())
      _destroyMachine();
  }

  unLisp *pushPrimitive(const String &name, Primitive *primitive)
  {
    mUserPrimitives.push(MakePair(name, primitive));
    return this;
  }

  void serializeNamesOfPrimitives(CBORArray *arr)
  {
    if (arr)
      mUserPrimitives.forEach([&](Pair<const String&, Primitive *> holder) {
        arr->put(holder.first.c_str());
      });
  }

  String popOutput() {
    return mOutputBuffer.popLimited("");
  }

  size_t sizeOutput() {
    return mOutputBuffer.size();
  }

  const String &getLastError()
  {
    return mLastError;
  }

private:
  unLisp()
  {
    auto fnPrintOut = [](const char *msg, int size) {
      if (size > 0)
      {
        auto &instance = unLisp::getInstance();
        auto alreadyFull = instance.mOutputBuffer.isFull();
        instance.mOutputBuffer.pushLimited(String(msg));
        instance.publish(Topic::LISP, alreadyFull ? Msg::MSG_REPLACED : Msg::MSG_ADDED);
      }
      yield();
    };

    auto fnPrintErr = [](const char *msg, int size) {
      auto &instance = unLisp::getInstance();
      instance.mLastError = msg;
      instance.publish(Topic::LISP, ERROR);
    };

    lisp_set_cycle_yield(yield);
    lisp_set_printers(fnPrintOut, fnPrintErr);

    _constructLispEnv();

    mOutputBuffer.limit(10);

    mTaskLispEval = TaskScheduler::make([this](short t) {
      auto root = mLispRoot;
      auto env = mLispEnv;

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
    lisp_create(8000);

    *mLispEnv = make_env(mLispRoot, &Nil, &Nil);
    define_constants(mLispRoot, mLispEnv);
    define_primitives(mLispRoot, mLispEnv);

    add_constant(mLispRoot, mLispEnv, "#t_obj", &Nil);
    add_constant_int(mLispRoot, mLispEnv, "#t_pass", 0);
    add_primitive(mLispRoot, mLispEnv, "task", mPrimitiveTask);

    mUserPrimitives.forEach([this](Pair<const String&, Primitive *> holder) {
      add_primitive(mLispRoot, mLispEnv, holder.first.c_str(), holder.second);
    });
  }

  void _destroyMachine()
  {
    lisp_destroy();
  }

  inline Object _primTask(Root root, VarObject env, VarObject list)
  {
    PrimitiveExpeditor expeditor("task", root, env, list);
    expeditor.assertArgs(3, Lisp::Int, Lisp::Int, Lisp::Cell);

    auto times = expeditor.getArgInt(0);
    auto ms = expeditor.getArgInt(1);
    auto obj = expeditor.getArg(2);

    UNIOT_LOG_DEBUG("%d", obj->type);

    DEFINE1(t_obj);
    *t_obj = get_variable(root, env, "#t_obj");
    (*t_obj)->cdr = obj;

    mTaskLispEval->attach(ms, times);
    return expeditor.makeBool(true);
  }

  Bytes mLastCode;
  String mLastError;
  LimitedQueue<String> mOutputBuffer;
  TaskScheduler::TaskPtr mTaskLispEval;
  ClearQueue<Pair<String, Primitive*>> mUserPrimitives;

  const Primitive *mPrimitiveTask = [](Root root, VarObject env, VarObject list) { return getInstance()._primTask(root, env, list); };

  void *mLispEnvConstructor[3];
  Root mLispRoot;
  VarObject mLispEnv;
};

} // namespace uniot
