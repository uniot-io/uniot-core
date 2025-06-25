#include <primitives.h>

Object filter_events(Root root, VarObject env, VarObject list) {
  auto expeditor = PrimitiveExpeditor::describe("filter_events", Lisp::Bool, 1, Lisp::Symbol)
                     .init(root, env, list);
  expeditor.assertDescribedArgs();
  String events = expeditor.getArgSymbol(0);

  if (events.isEmpty()) {
    return expeditor.makeBool(false);
  }

  UNIOT_LOG_INFO("Filtering events: %s", events.c_str());

  static ClearQueue<String> allowedEvents;
  allowedEvents.clean();

  int startPos = 0;
  int colonPos = events.indexOf(':');

  while (colonPos >= 0) {
    String eventName = events.substring(startPos, colonPos);
    if (eventName.length() > 0) {
      allowedEvents.push(eventName);
      UNIOT_LOG_INFO("Added event filter: %s", eventName.c_str());
    }
    startPos = colonPos + 1;
    colonPos = events.indexOf(':', startPos);
  }

  String lastEvent = events.substring(startPos);
  if (lastEvent.length() > 0) {
    allowedEvents.push(lastEvent);
    UNIOT_LOG_INFO("Added event filter: %s", lastEvent.c_str());
  }

  Uniot.setLispEventInterceptor([](const LispEvent& event) {
    if (allowedEvents.isEmpty()) {
      return true;
    }

    if (allowedEvents.contains(event.eventID)) {
      UNIOT_LOG_TRACE("Allowing event: %s", event.eventID.c_str());
      return true;
    }

    UNIOT_LOG_TRACE("Filtering out event: %s", event.eventID.c_str());
    return false;
  });

  return expeditor.makeBool(true);
}
