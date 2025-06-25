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

/** @cond */
/**
 * DO NOT DELETE THE "uniot-lisp" GROUP DEFINITION BELOW.
 * Used to create the Uniot Lisp topic in the documentation. If you want to delete this file,
 * please paste the group definition into another file and delete this one.
 */
/** @endcond */

/**
 * @defgroup uniot-lisp Uniot Lisp
 * @brief A wrapper for the Uniot Lisp interpreter
 *
 * This module provides a wrapper around the Uniot Lisp interpreter, allowing
 * for easy integration and usage within the Uniot Core. It manages the
 * lifecycle of the Lisp machine, and provides utilities for describing,
 * validating, and accessing Lisp primitive functions.
 */

#pragma once

#include <Bytes.h>
#include <CBORObject.h>
#include <Common.h>
#include <EventListener.h>
#include <LimitedQueue.h>
#include <LispEvents.h>
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

class IncomingEventManager {
 public:
  struct IncomingEvent {
    int32_t value;
    int8_t errorCode;

    IncomingEvent() : value(0), errorCode(0) {}
    IncomingEvent(int32_t v, int8_t err = 0) : value(v), errorCode(err) {}
  };

  void pushEvent(const Bytes &eventData) {
    CBORObject eventObj(eventData);
    auto eventID = eventObj.getString("eventID");
    auto valueStr = eventObj.getValueAsString("value");

    if (eventID.isEmpty() || valueStr.isEmpty()) {
      return;  // Invalid event
    }

    auto value = valueStr.toInt();
    auto isNumber = value || valueStr == "0";
    if (!isNumber) {
      return;  // Invalid event
    }

    bool isNewEvent = !mIncomingEvents.exist(eventID);
    if (isNewEvent) {
      auto eventQueue = std::make_shared<EventQueue>();
      mIncomingEvents.put(eventID, eventQueue);
      UNIOT_LOG_DEBUG("created new event queue for '%s'", eventID.c_str());
    }

    auto eventQueue = mIncomingEvents.get(eventID);
    eventQueue->lastAccessed = millis();

    size_t queueSizeBefore = eventQueue->queue.size();
    eventQueue->queue.pushLimited(IncomingEvent(value));
    size_t queueSizeAfter = eventQueue->queue.size();

    if (queueSizeAfter > queueSizeBefore) {
      UNIOT_LOG_TRACE("pushed event '%s' with value '%d', queue size: %d", eventID.c_str(), value, queueSizeAfter);
    } else {
      UNIOT_LOG_WARN("event queue for '%s' is full (limit: %d), oldest event was dropped", eventID.c_str(), EVENTS_LIMIT);
    }
  }

  bool isEventAvailable(const String &eventID) {
    if (!mIncomingEvents.exist(eventID)) {
      return false;
    }

    auto eventQueue = mIncomingEvents.get(eventID);
    eventQueue->isUsedInScript = true;
    eventQueue->lastAccessed = millis();

    return eventQueue->queue.size() > 0;
  }

  IncomingEvent popEvent(const String &eventID) {
    const IncomingEvent emptyEvent(0, -1);

    if (!mIncomingEvents.exist(eventID)) {
      UNIOT_LOG_WARN("attempted to pop non-existent event '%s'", eventID.c_str());
      return emptyEvent;
    }

    auto eventQueue = mIncomingEvents.get(eventID);
    eventQueue->isUsedInScript = true;
    eventQueue->lastAccessed = millis();

    if (eventQueue->queue.size() == 0) {
      UNIOT_LOG_WARN("attempted to pop from empty event queue '%s'", eventID.c_str());
      return emptyEvent;
    }

    return eventQueue->queue.popLimited(emptyEvent);
  }

  void cleanupUnusedEvents() {
    unsigned long currentTime = millis();
    ClearQueue<String> keysToRemove;
    size_t totalEvents = 0;
    size_t unusedEvents = 0;
    size_t expiredEvents = 0;
    size_t totalQueuedItems = 0;

    mIncomingEvents.forEach([&](const Pair<String, SharedPointer<EventQueue>> &entry) {
      totalEvents++;
      auto eventQueue = entry.second;
      totalQueuedItems += eventQueue->queue.size();
      unsigned long timeSinceLastAccess = currentTime - eventQueue->lastAccessed;

      if (!eventQueue->isUsedInScript) {
        unusedEvents++;
        if (timeSinceLastAccess > EVENT_TTL_MS) {
          expiredEvents++;
          keysToRemove.push(entry.first);
          UNIOT_LOG_DEBUG("marking event '%s' for removal (unused, last accessed %lu ms ago)",
                          entry.first.c_str(), timeSinceLastAccess);
        }
      }
    });

    // Remove expired events
    keysToRemove.forEach([this](const String &key) {
      mIncomingEvents.remove(key);
    });

    if (expiredEvents > 0) {
      UNIOT_LOG_INFO("cleaned up %d expired events (total: %d, unused: %d, queued items: %d)",
                     expiredEvents, totalEvents, unusedEvents, totalQueuedItems);
    } else if (totalEvents > 0) {
      UNIOT_LOG_TRACE("no events to cleanup (total: %d, unused: %d, queued items: %d)", totalEvents, unusedEvents, totalQueuedItems);
    }
  }

  void clean() {
    mIncomingEvents.clean();
    UNIOT_LOG_DEBUG("cleared all incoming events");
  }

 private:
  struct EventQueue {
    LimitedQueue<IncomingEvent> queue;
    unsigned long lastAccessed;
    bool isUsedInScript;

    EventQueue() : lastAccessed(millis()), isUsedInScript(false) {
      queue.limit(EVENTS_LIMIT);
    }
  };

  Map<String, SharedPointer<EventQueue>> mIncomingEvents;
  static constexpr size_t EVENTS_LIMIT = 2;
  static constexpr unsigned long EVENT_TTL_MS = 30000;  // 30 seconds TTL
};

/**
 * @brief A singleton class that wraps UniotLisp interpreter for the Uniot Core.
 * @defgroup unlisp unLisp
 * @ingroup uniot-lisp
 *
 * unLisp provides a Lisp environment with event-based communication capabilities
 * for IoT applications. It manages the Lisp machine lifecycle, handles user-defined
 * primitive functions, and provides an event system for communication between the
 * Lisp environment and the rest of the application.
 *
 * The class supports:
 * - Running Lisp code in a managed environment
 * - Custom primitive functions via C++ callbacks
 * - Event-based communication between Lisp and the host application
 * - Task scheduling for periodic Lisp function execution
 * @{
 */
class unLisp : public CoreEventListener, public Singleton<unLisp> {
  friend class Singleton<unLisp>;

 public:
  /**
   * @brief Get the task associated with Lisp evaluation
   * @retval TaskScheduler::TaskPtr The task pointer for the Lisp evaluation
   */
  TaskScheduler::TaskPtr getTask() {
    return mTaskLispEval;
  }

  TaskScheduler::TaskPtr getCleanupTask() {
    return mTaskEventCleanup;
  }

  /**
   * @brief Check if the Lisp machine has been created
   * @retval true The Lisp machine exists
   * @retval false The Lisp machine does not exist
   */
  bool isCreated() {
    return lisp_is_created();
  }

  /**
   * @brief Check if the Lisp evaluation task is currently running
   * @retval true The task is running (attached)
   * @retval false The task is not running
   */
  bool taskIsRunning() {
    return mTaskLispEval->isAttached();
  }

  /**
   * @brief Get the amount of memory used by the Lisp machine
   * @retval memoryUsed Size in bytes of memory used
   */
  size_t memoryUsed() {
    return lisp_mem_used();
  }

  /**
   * @brief Run Lisp code in the interpreter
   *
   * This method creates a fresh Lisp machine if needed, stores the code,
   * refreshes event queues, evaluates the code, and manages machine lifecycle.
   *
   * @param data The Lisp code to execute as a byte array
   */
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

  /**
   * @brief Add a new primitive function to the Lisp environment
   *
   * @param primitive Pointer to the primitive function
   * @retval unLisp* Pointer to the unLisp instance
   */
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

  /**
   * @brief Serialize all registered primitive functions to CBOR format
   *
   * This allows the runtime to expose information about available primitives
   * to client applications or documentation systems.
   *
   * @param obj CBOR object to populate with primitive information
   */
  void serializePrimitives(CBORObject &obj) {
    mUserPrimitives.forEach([&](Pair<const String &, Primitive *> holder) {
      auto description = PrimitiveExpeditor::extractDescription(holder.second);
      obj.putArray(holder.first.c_str())
          .append(description.returnType)
          .appendArray()
          .append(description.argsCount, reinterpret_cast<const uint8_t *>(description.argsTypes));
    });
  }

  /**
   * @brief Get the last executed Lisp code
   * @retval Bytes The last executed code
   */
  const Bytes &getLastCode() {
    UNIOT_LOG_WARN_IF(!mLastCode.size(), "there is no last saved code");
    return mLastCode;
  }

  /**
   * @brief Clear the stored last executed code
   */
  void cleanLastCode() {
    mLastCode.clean();
  }

  /**
   * @brief Event handler for received events
   *
   * This method processes events directed to the Lisp environment,
   * particularly handling incoming events from the application.
   *
   * @param topic The topic of the received event
   * @param msg The specific message type
   */
  virtual void onEventReceived(unsigned int topic, int msg) override {
    if (topic == events::lisp::Topic::IN_LISP_EVENT) {
      if (msg == events::lisp::Msg::IN_NEW_EVENT) {
        CoreEventListener::receiveDataFromChannel(events::lisp::Channel::IN_EVENT, [&](unsigned int id, bool empty, Bytes data) {
          _pushIncomingEvent(data);
        });
        return;
      }
      return;
    }
  }

 private:
  /**
   * @brief Constructor setting up the Lisp environment and callbacks
   *
   * Initializes the event listeners, sets up output handlers, and creates
   * the evaluation task for the Lisp machine.
   */
  unLisp() {
    CoreEventListener::listenToEvent(events::lisp::Topic::IN_LISP_EVENT);

    auto fnPrintOut = [](const char *msg, int size) {
      if (size > 0) {
        auto &instance = unLisp::getInstance();
        instance.sendDataToChannel(events::lisp::Channel::OUT_LISP, Bytes((uint8_t *)msg, size).terminate());
        instance.emitEvent(events::lisp::Topic::OUT_LISP_MSG, events::lisp::Msg::OUT_MSG_ADDED);
      }
      yield();
    };

    auto fnPrintLog = [](const char *msg, int size) {
      if (size > 0) {
        auto &instance = unLisp::getInstance();
        instance.sendDataToChannel(events::lisp::Channel::OUT_LISP_LOG, Bytes((uint8_t *)msg, size).terminate());
        instance.emitEvent(events::lisp::Topic::OUT_LISP_MSG, events::lisp::Msg::OUT_MSG_LOG);
      }
      yield();
    };

    auto fnPrintErr = [](const char *msg, int size) {
      auto &instance = unLisp::getInstance();
      instance.sendDataToChannel(events::lisp::Channel::OUT_LISP_ERR, Bytes((uint8_t *)msg, size).terminate());
      instance.emitEvent(events::lisp::Topic::OUT_LISP_MSG, events::lisp::Msg::OUT_MSG_ERROR);

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

    mTaskEventCleanup = TaskScheduler::make([this](SchedulerTask &self, short t) {
      mEventManager.cleanupUnusedEvents();
      yield();
    });
  }

  /**
   * @brief Initialize the Lisp environment constructor
   *
   * Sets up the basic structure for the Lisp environment.
   */
  void _constructLispEnv() {
    mLispEnvConstructor[0] = NULL;
    mLispEnvConstructor[1] = NULL;
    mLispEnvConstructor[2] = ROOT_END;
    mLispRoot = mLispEnvConstructor;
    mLispEnv = (Obj **)(mLispEnvConstructor + 1);
  }

  /**
   * @brief Create a new Lisp machine
   *
   * Initializes the Lisp heap, sets up constants, primitives, and
   * registers user-defined primitives in the environment.
   */
  void _createMachine() {
    lisp_create(UNIOT_LISP_HEAP);

    *mLispEnv = make_env(mLispRoot, &Nil, &Nil);
    define_constants(mLispRoot, mLispEnv);
    define_primitives(mLispRoot, mLispEnv);

    // Register special variables and built-in primitives
    add_constant(mLispRoot, mLispEnv, "#t_obj", &Nil);
    add_constant_int(mLispRoot, mLispEnv, "#t_pass", 0);
    add_primitive(mLispRoot, mLispEnv, "task", mPrimitiveTask);
    add_primitive(mLispRoot, mLispEnv, "is_event", mPrimitiveIsEventAvailable);
    add_primitive(mLispRoot, mLispEnv, "pop_event", mPrimitivePopEvent);
    add_primitive(mLispRoot, mLispEnv, "push_event", mPrimitivePushEvent);

    // Register all user-defined primitives
    mUserPrimitives.forEach([this](Pair<const String &, Primitive *> holder) {
      add_primitive(mLispRoot, mLispEnv, holder.first.c_str(), holder.second);
    });

    UNIOT_LOG_DEBUG("lisp machine created, mem used: %d", lisp_mem_used());
  }

  /**
   * @brief Destroy the Lisp machine and free its resources
   */
  void _destroyMachine() {
    lisp_destroy();
  }

  /**
   * @brief Clear the incoming events queue and request event refresh
   */
  void _refreshIncomingEvents() {
    mEventManager.clean();
    this->emitEvent(events::lisp::Topic::OUT_LISP_REQUEST, events::lisp::Msg::OUT_REFRESH_EVENTS);
  }

  /**
   * @brief Add an incoming event to the appropriate event queue
   *
   * @param eventData Event data in CBOR format
   */
  void _pushIncomingEvent(const Bytes &eventData) {
    // mEventManager.cleanupUnusedEvents();
    mEventManager.pushEvent(eventData);
  }

  /**
   * @brief Check if an event with the given ID is available
   *
   * @param eventID The ID of the event to check
   * @retval true The event is available
   * @retval false The event is not available
   */
  bool _isIncomingEventAvailable(const String &eventID) {
    return mEventManager.isEventAvailable(eventID);
  }

  /**
   * @brief Check if an event with the given ID is available
   *
   * @param eventID The ID of the event to check
   * @retval true The event is available
   * @retval false The event is not available
   */
  IncomingEventManager::IncomingEvent _popIncomingEvent(const String &eventID) {
    return mEventManager.popEvent(eventID);
  }

  /**
   * @brief Send an event from Lisp to the application
   *
   * @param eventID The ID of the event
   * @param value The integer value to send with the event
   * @retval true The event was sent
   * @retval false The event was not sent
   */
  bool _pushOutgoingEvent(String eventID, int value) {
    CBORObject event;
    event.put("eventID", eventID.c_str());
    event.put("value", value);

    auto sent = this->sendDataToChannel(events::lisp::Channel::OUT_EVENT, event.build());
    this->emitEvent(events::lisp::Topic::OUT_LISP_EVENT, events::lisp::Msg::OUT_NEW_EVENT);
    return sent;
  }

  /**
   * @brief Lisp primitive for scheduling a task
   *
   * Usage from Lisp: (task times ms expression)
   * Schedules a Lisp expression to run periodically.
   *
   * @param root The Lisp root object
   * @param env The current environment
   * @param list The arguments list from Lisp
   * @retval bool A value indicating if the task was successfully scheduled
   */
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

  /**
   * @brief Lisp primitive to check if an event is available
   *
   * Usage from Lisp: (is_event eventID)
   *
   * @param root The Lisp root object
   * @param env The current environment
   * @param list The arguments list from Lisp
   * @retval bool A value indicating if an event with the given ID is available
   */
  inline Object _primIsEventAvailable(Root root, VarObject env, VarObject list) {
    auto expeditor = PrimitiveExpeditor::describe("is_event", Lisp::Bool, 1, Lisp::Symbol)
                         .init(root, env, list);
    expeditor.assertDescribedArgs();

    auto eventId = expeditor.getArgSymbol(0);
    auto available = _isIncomingEventAvailable(eventId);

    return expeditor.makeBool(available);
  }

  /**
   * @brief Lisp primitive to get and remove an event
   *
   * Usage from Lisp: (pop_event eventID)
   * Gets and removes the next event with the given ID.
   *
   * @param root The Lisp root object
   * @param env The current environment
   * @param list The arguments list from Lisp
   * @retval int The event value
   * @retval 0 No event available
   */
  inline Object _primPopEvent(Root root, VarObject env, VarObject list) {
    auto expeditor = PrimitiveExpeditor::describe("pop_event", Lisp::Bool, 1, Lisp::Symbol)
                         .init(root, env, list);
    expeditor.assertDescribedArgs();

    auto eventId = expeditor.getArgSymbol(0);
    auto event = _popIncomingEvent(eventId);

    UNIOT_LOG_WARN_IF(event.errorCode, "error popping event '%s': %d", event.errorCode);

    return expeditor.makeInt(event.value);
  }

  /**
   * @brief Lisp primitive to send an event to the application
   *
   * Usage from Lisp: (push_event eventID value)
   * Sends an event with the given ID and value to the application.
   *
   * @param root The Lisp root object
   * @param env The current environment
   * @param list The arguments list from Lisp
   * @retval bool A value indicating if the event was successfully sent
   */
  inline Object _primPushEvent(Root root, VarObject env, VarObject list) {
    auto expeditor = PrimitiveExpeditor::describe("push_event", Lisp::Bool, 2, Lisp::Symbol, Lisp::BoolInt)
                         .init(root, env, list);
    expeditor.assertDescribedArgs();

    auto eventId = expeditor.getArgSymbol(0);
    auto value = expeditor.getArgInt(1);

    auto sent = _pushOutgoingEvent(eventId, value);

    return expeditor.makeBool(sent);
  }

  // Private member variables
  Bytes mLastCode;                       ///< Storage for the last executed Lisp code
  TaskScheduler::TaskPtr mTaskLispEval;  ///< Task for scheduled Lisp evaluation
  TaskScheduler::TaskPtr mTaskEventCleanup;
  ClearQueue<Pair<String, Primitive *>> mUserPrimitives;  ///< Queue of user-defined primitives

  /**
   * @brief Primitive function pointers for built-in Lisp functions
   */
  const Primitive *mPrimitiveTask = [](Root root, VarObject env, VarObject list) { return getInstance()._primTask(root, env, list); };
  const Primitive *mPrimitiveIsEventAvailable = [](Root root, VarObject env, VarObject list) { return getInstance()._primIsEventAvailable(root, env, list); };
  const Primitive *mPrimitivePopEvent = [](Root root, VarObject env, VarObject list) { return getInstance()._primPopEvent(root, env, list); };
  const Primitive *mPrimitivePushEvent = [](Root root, VarObject env, VarObject list) { return getInstance()._primPushEvent(root, env, list); };

  void *mLispEnvConstructor[3];        ///< Storage for Lisp environment constructor
  Root mLispRoot;                      ///< Lisp root object
  VarObject mLispEnv;                  ///< Lisp environment object
  IncomingEventManager mEventManager;  ///< Map of event queues by event ID
};
/** @} */
}  // namespace uniot
