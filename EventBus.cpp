#pragma once
#include "EventBus.h"

EventListener* EventBus::_listeners[EventBus::MAX_LISTENERS] = {nullptr};

void EventBus::subscribe(EventListener* listener) {
    if (listener == nullptr) return;

    // Check if already subscribed
    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (_listeners[i] == listener) return;
    }

    // Find empty slot
    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (_listeners[i] == nullptr) {
            _listeners[i] = listener;
            return;
        }
    }
}

void EventBus::unsubscribe(EventListener* listener) {
    if (listener == nullptr) return;

    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (_listeners[i] == listener) {
            _listeners[i] = nullptr;
            return;
        }
    }
}

void EventBus::publish(SystemEvent event) {
    for (int i = 0; i < MAX_LISTENERS; i++) {
        if (_listeners[i] != nullptr) {
            _listeners[i]->onEvent(event);
        }
    }
}
