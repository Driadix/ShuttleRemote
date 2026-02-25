#pragma once
#include <Arduino.h>
#include "InputEvents.h"
#include "EventBus.h"
#include "ShuttleProtocolTypes.h"
#include "CommLink.h"

class DebugUtils {
public:
    static const char* getEventName(InputEvent event);
    static const char* getSystemEventName(SystemEvent event);
    static const char* getMsgIdName(uint8_t msgId);
    static const char* getTxStateName(CommLink::TxState state);
};
