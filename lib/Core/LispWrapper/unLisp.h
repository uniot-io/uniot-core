/*
 * This is a part of the Uniot project.
 * Copyright (C) 2016-2023 Uniot <contact@uniot.io>
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

#include <Bytes.h>
#include <CBORArray.h>
#include <Common.h>
#include <EventListener.h>
#include <LimitedQueue.h>
#include <LispHelper.h>
#include <Logger.h>
#include <PrimitiveExpeditor.h>
#include <TaskScheduler.h>
#include <libminilisp.h>

#ifndef UNIOT_LISP_HEAP
#define UNIOT_LISP_HEAP 8000
#endif

namespace uniot {
using namespace lisp;

class unLisp : public CoreEventListener {
 public:
  enum Channel {
    LISP_OUTPUT = FOURCC(lout),
    EVENT = FOURCC(evnt)
  };
  enum Topic {
    LISP_OUT = FOURCC(lisp),
    LISP_EVENT = FOURCC(lspe)
  };
  enum Msg {
    MSG_ADDED,
    ERROR,
    NEW_EVENT
  };

  unLisp(unLisp const &) = delete;
  void operator=(unLisp const &) = delete;

  static unLisp &getInstance() {
    static unLisp instance;
    return instance;
  }

  TaskScheduler::TaskPtr getTask() {
    return mTaskLispEval;
  }

  bool isCreated() {
    return lisp_is_created();
  }

  bool taskIsRunning() {
    return mTaskLispEval->isAtached();
  }

  size_t memoryUsed() {
    return lisp_mem_used();
  }

  void runCode(const Bytes &data) {
    if (!data.size())
      return;

    mIsLastCodePersist = false;
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

  unLisp *pushPrimitive(const String &name, Primitive *primitive) {
    mUserPrimitives.push(MakePair(name, primitive));
    return this;
  }

  void serializeNamesOfPrimitives(CBORArray *arr) {
    if (arr)
      mUserPrimitives.forEach([&](Pair<const String &, Primitive *> holder) {
        arr->put(holder.first.c_str());
      });
  }

  const String &getLastError() {
    return mLastError;
  }

  const Bytes &getLastCode() {
    UNIOT_LOG_WARN_IF(!mLastCode.size(), "there is no last saved code");
    return mLastCode;
  }

  bool isLastCodePersist() {
    return mIsLastCodePersist;
  }

  void cleanLastCode() {
    mLastCode.clean();
  }

  virtual void onEventReceived(unsigned int topic, int msg) override {
    UNIOT_LOG_INFO("Received event: %d, %d", topic, msg);

    CoreEventListener::receiveDataFromChannel(unLisp::Channel::EVENT, [&](unsigned int id, bool empty, Bytes data) {
      UNIOT_LOG_INFO("event bus id: %d, channel empty: %d, event size: %d", id, empty, data.checksum());

      auto packet = CBORObject(data);
      auto eventID = packet.getString("eventID");
      auto value = packet.getValueAsString("value");
      auto sender = packet.getMap("sender").getString("id");
      UNIOT_LOG_INFO("eventID: %s, value: %s, sender: %s", eventID.c_str(), value.c_str(), sender.c_str());
    });
  }

 private:
  unLisp() : mIsLastCodePersist(false) {
    CoreEventListener::listenToEvent(unLisp::Topic::LISP_EVENT);

    auto fnPrintOut = [](const char *msg, int size) {
      if (size > 0) {
        auto &instance = unLisp::getInstance();
        instance.sendDataToChannel(Topic::LISP_OUT, Bytes((uint8_t *)msg, size).terminate());
        instance.emitEvent(Topic::LISP_OUT, Msg::MSG_ADDED);
      }
      yield();
    };

    auto fnPrintErr = [](const char *msg, int size) {
      auto &instance = unLisp::getInstance();
      instance.mLastError = msg;
      instance.emitEvent(Topic::LISP_OUT, ERROR);

      // TODO: do we need a special error callback?
      instance.mTaskLispEval->detach();
      instance._destroyMachine();
    };

    lisp_set_cycle_yield(yield);
    lisp_set_printers(fnPrintOut, fnPrintErr);

    _constructLispEnv();

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

  void _constructLispEnv() {
    mLispEnvConstructor[0] = NULL;
    mLispEnvConstructor[1] = NULL;
    mLispEnvConstructor[2] = ROOT_END;
    mLispRoot = mLispEnvConstructor;
    mLispEnv = (Obj **)(mLispEnvConstructor + 1);
  }

  void _createMachine() {
    lisp_create(UNIOT_LISP_HEAP);

    *mLispEnv = make_env(mLispRoot, &Nil, &Nil);
    define_constants(mLispRoot, mLispEnv);
    define_primitives(mLispRoot, mLispEnv);

    add_constant(mLispRoot, mLispEnv, "#t_obj", &Nil);
    add_constant_int(mLispRoot, mLispEnv, "#t_pass", 0);
    add_primitive(mLispRoot, mLispEnv, "task", mPrimitiveTask);

    mUserPrimitives.forEach([this](Pair<const String &, Primitive *> holder) {
      add_primitive(mLispRoot, mLispEnv, holder.first.c_str(), holder.second);
    });
  }

  void _destroyMachine() {
    lisp_destroy();
  }

  inline Object _primTask(Root root, VarObject env, VarObject list) {
    PrimitiveExpeditor expeditor("task", root, env, list);
    expeditor.assertArgs(3, Lisp::Int, Lisp::Int, Lisp::Cell);

    auto times = expeditor.getArgInt(0);
    auto ms = expeditor.getArgInt(1);
    auto obj = expeditor.getArg(2);

    DEFINE1(t_obj);
    *t_obj = get_variable(root, env, "#t_obj");
    (*t_obj)->cdr = obj;

    mTaskLispEval->attach(ms, times);
    mIsLastCodePersist = times <= 0;

    return expeditor.makeBool(true);
  }

  bool mIsLastCodePersist;
  Bytes mLastCode;
  String mLastError;
  TaskScheduler::TaskPtr mTaskLispEval;
  ClearQueue<Pair<String, Primitive *>> mUserPrimitives;

  const Primitive *mPrimitiveTask = [](Root root, VarObject env, VarObject list) { return getInstance()._primTask(root, env, list); };

  void *mLispEnvConstructor[3];
  Root mLispRoot;
  VarObject mLispEnv;
};

}  // namespace uniot
