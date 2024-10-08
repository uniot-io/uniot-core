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
#include <CBORObject.h>
#include <Common.h>
#include <EventListener.h>
#include <LimitedQueue.h>
#include <LispHelper.h>
#include <Logger.h>
#include <PrimitiveExpeditor.h>
#include <Singleton.h>
#include <TaskScheduler.h>
#include <libminilisp.h>

#ifndef UNIOT_LISP_HEAP
#define UNIOT_LISP_HEAP 8000
#endif

namespace uniot {
using namespace lisp;

class unLisp : public CoreEventListener, public Singleton<unLisp> {
  friend class Singleton<unLisp>;

 public:
  enum Channel {
    OUT_LISP = FOURCC(lout),
    OUT_LISP_LOG = FOURCC(llog),
    OUT_LISP_ERR = FOURCC(lerr),
    OUT_EVENT = FOURCC(evou),
    IN_EVENT = FOURCC(evin),
  };
  enum Topic {
    OUT_LISP_MSG = FOURCC(lisp),
    OUT_LISP_REQUEST = FOURCC(lspr),
    OUT_LISP_EVENT = FOURCC(levo),
    IN_LISP_EVENT = FOURCC(levi)
  };
  enum Msg {
    OUT_MSG_ADDED,
    OUT_MSG_LOG,
    OUT_MSG_ERROR,
    OUT_REFRESH_EVENTS,
    OUT_NEW_EVENT,
    IN_NEW_EVENT
  };

  TaskScheduler::TaskPtr getTask() {
    return mTaskLispEval;
  }

  bool isCreated() {
    return lisp_is_created();
  }

  bool taskIsRunning() {
    return mTaskLispEval->isAttached();
  }

  size_t memoryUsed() {
    return lisp_mem_used();
  }

  void runCode(const Bytes &data) {
    if (!data.size())
      return;

    mLastCode = data;

    mTaskLispEval->detach();
    _destroyMachine();
    _createMachine();

    auto code = mLastCode.terminate().c_str();
    UNIOT_LOG_DEBUG("eval: %s", code);

    _refreshIncomingEvents();
    lisp_eval(mLispRoot, mLispEnv, code);
    if (!mTaskLispEval->isAttached()) {
      _destroyMachine();
    }
  }

  unLisp *pushPrimitive(Primitive *primitive) {
    auto description = PrimitiveExpeditor::extractDescription(primitive);
    mUserPrimitives.push(MakePair(description.name, primitive));
    UNIOT_LOG_TRACE("primitive added: %s", description.name.c_str());
    UNIOT_LOG_TRACE("args count: %d", description.argsCount);
    UNIOT_LOG_TRACE("return type: %d", description.returnType);
    for (auto i = 0; i < description.argsCount; i++) {
      UNIOT_LOG_TRACE("arg %d: %d", i, description.argsTypes[i]);
    }
    return this;
  }

  void serializePrimitives(CBORObject &obj) {
    mUserPrimitives.forEach([&](Pair<const String &, Primitive *> holder) {
      auto description = PrimitiveExpeditor::extractDescription(holder.second);
      obj.putArray(holder.first.c_str())
          .append(description.returnType)
          .appendArray()
          .append(description.argsCount, reinterpret_cast<const uint8_t *>(description.argsTypes));
    });
  }

  const Bytes &getLastCode() {
    UNIOT_LOG_WARN_IF(!mLastCode.size(), "there is no last saved code");
    return mLastCode;
  }

  void cleanLastCode() {
    mLastCode.clean();
  }

  virtual void onEventReceived(unsigned int topic, int msg) override {
    if (topic == unLisp::Topic::IN_LISP_EVENT) {
      if (msg == unLisp::Msg::IN_NEW_EVENT) {
        CoreEventListener::receiveDataFromChannel(unLisp::Channel::IN_EVENT, [&](unsigned int id, bool empty, Bytes data) {
          _pushIncomingEvent(data);
        });
        return;
      }
      return;
    }
  }

 private:
  unLisp() {
    CoreEventListener::listenToEvent(unLisp::Topic::IN_LISP_EVENT);

    auto fnPrintOut = [](const char *msg, int size) {
      if (size > 0) {
        auto &instance = unLisp::getInstance();
        instance.sendDataToChannel(unLisp::Channel::OUT_LISP, Bytes((uint8_t *)msg, size).terminate());
        instance.emitEvent(Topic::OUT_LISP_MSG, Msg::OUT_MSG_ADDED);
      }
      yield();
    };

    auto fnPrintLog = [](const char *msg, int size) {
      if (size > 0) {
        auto &instance = unLisp::getInstance();
        instance.sendDataToChannel(unLisp::Channel::OUT_LISP_LOG, Bytes((uint8_t *)msg, size).terminate());
        instance.emitEvent(Topic::OUT_LISP_MSG, Msg::OUT_MSG_LOG);
      }
      yield();
    };

    auto fnPrintErr = [](const char *msg, int size) {
      auto &instance = unLisp::getInstance();
      instance.sendDataToChannel(unLisp::Channel::OUT_LISP_ERR, Bytes((uint8_t *)msg, size).terminate());
      instance.emitEvent(Topic::OUT_LISP_MSG, OUT_MSG_ERROR);

      instance.mTaskLispEval->detach();
      instance._destroyMachine();
    };

    lisp_set_cycle_yield(yield);
    lisp_set_printers(fnPrintOut, fnPrintLog, fnPrintErr);

    _constructLispEnv();

    mTaskLispEval = TaskScheduler::make([this](SchedulerTask &self, short t) {
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

      // UNIOT_LOG_DEBUG("lisp machine running, mem used: %d", lisp_mem_used());
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
    add_primitive(mLispRoot, mLispEnv, "is_event", mPrimitiveIsEventAvailable);
    add_primitive(mLispRoot, mLispEnv, "pop_event", mPrimitivePopEvent);
    add_primitive(mLispRoot, mLispEnv, "push_event", mPrimitivePushEvent);

    mUserPrimitives.forEach([this](Pair<const String &, Primitive *> holder) {
      add_primitive(mLispRoot, mLispEnv, holder.first.c_str(), holder.second);
    });

    UNIOT_LOG_DEBUG("lisp machine created, mem used: %d", lisp_mem_used());
  }

  void _destroyMachine() {
    lisp_destroy();
  }

  void _refreshIncomingEvents() {
    mIncomingEvents.clean();
    this->emitEvent(Topic::OUT_LISP_REQUEST, Msg::OUT_REFRESH_EVENTS);
  }

  void _pushIncomingEvent(const Bytes &eventData) {
    constexpr size_t EVENTS_LIMIT = 5;

    CBORObject event(eventData);
    auto eventID = event.getString("eventID");
    auto value = event.getValueAsString("value");  // NOTE: this is only used here to verify the correctness of the event

    if (!eventID.isEmpty() && !value.isEmpty()) {
      if (!mIncomingEvents.exist(eventID)) {
        mIncomingEvents.put(eventID, std::make_shared<LimitedQueue<Bytes>>());
        mIncomingEvents.get(eventID)->limit(EVENTS_LIMIT);
      }
      mIncomingEvents.get(eventID)->pushLimited(eventData);
    }

    // TODO: Find a way to remove events that are not consumed
  }

  bool _isIncomingEventAvailable(const String &eventID) {
    return mIncomingEvents.exist(eventID) && mIncomingEvents.get(eventID)->size();
  }

  Bytes _popIncomingEvent(const String &eventID) {
    if (_isIncomingEventAvailable(eventID)) {
      return mIncomingEvents.get(eventID)->popLimited({});
    }
    return {};
  }

  bool _pushOutgoingEvent(String eventID, int value) {
    CBORObject event;
    event.put("eventID", eventID.c_str());
    event.put("value", value);

    auto sent = this->sendDataToChannel(unLisp::Channel::OUT_EVENT, event.build());
    this->emitEvent(Topic::OUT_LISP_EVENT, Msg::OUT_NEW_EVENT);
    return sent;
  }

  inline Object _primTask(Root root, VarObject env, VarObject list) {
    auto expeditor = PrimitiveExpeditor::describe("task", Lisp::Bool, 3, Lisp::Int, Lisp::Int, Lisp::Cell)
                         .init(root, env, list);
    expeditor.assertDescribedArgs();

    auto times = expeditor.getArgInt(0);
    auto ms = expeditor.getArgInt(1);
    auto obj = expeditor.getArg(2);

    DEFINE1(t_obj);
    *t_obj = get_variable(root, env, "#t_obj");
    (*t_obj)->cdr = obj;

    mTaskLispEval->attach(ms, times);

    return expeditor.makeBool(true);
  }

  inline Object _primIsEventAvailable(Root root, VarObject env, VarObject list) {
    auto expeditor = PrimitiveExpeditor::describe("is_event", Lisp::Bool, 1, Lisp::Symbol)
                         .init(root, env, list);
    expeditor.assertDescribedArgs();

    auto eventId = expeditor.getArgSymbol(0);
    auto available = _isIncomingEventAvailable(eventId);

    return expeditor.makeBool(available);
  }

  inline Object _primPopEvent(Root root, VarObject env, VarObject list) {
    auto expeditor = PrimitiveExpeditor::describe("pop_event", Lisp::Bool, 1, Lisp::Symbol)
                         .init(root, env, list);
    expeditor.assertDescribedArgs();

    auto eventId = expeditor.getArgSymbol(0);
    auto eventData = _popIncomingEvent(eventId);
    CBORObject event(eventData);
    auto value = event.getValueAsString("value");

    auto number = value.toInt();
    auto isNumber = number || value == "0";

    UNIOT_LOG_WARN_IF(!isNumber, "event value is not a number");

    return expeditor.makeInt(number);
  }

  inline Object _primPushEvent(Root root, VarObject env, VarObject list) {
    auto expeditor = PrimitiveExpeditor::describe("push_event", Lisp::Bool, 2, Lisp::Symbol, Lisp::BoolInt)
                         .init(root, env, list);
    expeditor.assertDescribedArgs();

    auto eventId = expeditor.getArgSymbol(0);
    auto value = expeditor.getArgInt(1);

    auto sent = _pushOutgoingEvent(eventId, value);

    return expeditor.makeBool(sent);
  }

  Bytes mLastCode;
  TaskScheduler::TaskPtr mTaskLispEval;
  ClearQueue<Pair<String, Primitive *>> mUserPrimitives;

  const Primitive *mPrimitiveTask = [](Root root, VarObject env, VarObject list) { return getInstance()._primTask(root, env, list); };
  const Primitive *mPrimitiveIsEventAvailable = [](Root root, VarObject env, VarObject list) { return getInstance()._primIsEventAvailable(root, env, list); };
  const Primitive *mPrimitivePopEvent = [](Root root, VarObject env, VarObject list) { return getInstance()._primPopEvent(root, env, list); };
  const Primitive *mPrimitivePushEvent = [](Root root, VarObject env, VarObject list) { return getInstance()._primPushEvent(root, env, list); };

  void *mLispEnvConstructor[3];
  Root mLispRoot;
  VarObject mLispEnv;
  Map<String, SharedPointer<LimitedQueue<Bytes>>> mIncomingEvents;  // TODO: TTL
};

}  // namespace uniot
