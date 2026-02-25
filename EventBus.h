#pragma once
#include <stdint.h>
#include <stddef.h>

// Event types
enum class SystemEvent {
    TELEMETRY_UPDATED,
    SENSORS_UPDATED,
    STATS_UPDATED,
    CONFIG_UPDATED,
    ERROR_OCCURRED,
    BATTERY_LOW,
    QUEUE_FULL,
    QUEUE_OK,
    CONNECTION_LOST,
    CONNECTION_RESTORED,
    LOCAL_BATT_UPDATED,
    CMD_DISPATCHED,
    CMD_ACKED,
    CMD_FAILED
};

// Interface for listeners
class EventListener {
public:
    virtual void onEvent(SystemEvent event) = 0;
};

// Simple Event Bus (Pub/Sub)
class EventBus {
public:
    static const int MAX_LISTENERS = 16;

    static void subscribe(EventListener* listener);
    static void unsubscribe(EventListener* listener);
    static void publish(SystemEvent event);

private:
    static EventListener* _listeners[MAX_LISTENERS];
};
