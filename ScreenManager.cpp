#include "ScreenManager.h"
#include <Arduino.h> // For debugging Serial if needed

ScreenManager& ScreenManager::getInstance() {
    static ScreenManager instance;
    return instance;
}

ScreenManager::ScreenManager() : _topIndex(-1) {
    // Initialize stack with nulls
    for (int i = 0; i < MAX_STACK_SIZE; i++) {
        _stack[i] = nullptr;
    }
}

void ScreenManager::push(Screen* s) {
    if (s == nullptr) return;

    if (_topIndex < MAX_STACK_SIZE - 1) {
        _topIndex++;
        _stack[_topIndex] = s;
        s->onEnter();
        s->setDirty(); // Ensure it draws
    } else {
        // Stack overflow: Pop oldest screen (except root at 0)
        // Shift 1..MAX-1 down to create space at top

        if (_stack[1] != nullptr) {
            _stack[1]->onExit();
        }

        for (int i = 1; i < MAX_STACK_SIZE - 1; i++) {
            _stack[i] = _stack[i+1];
        }

        // Push to top slot
        _stack[MAX_STACK_SIZE - 1] = s;
        // _topIndex remains at MAX

        s->onEnter();
        s->setDirty();
    }
}

void ScreenManager::pop() {
    if (_topIndex >= 0) {
        _stack[_topIndex]->onExit();
        _stack[_topIndex] = nullptr; // Clear the pointer
        _topIndex--;

        if (_topIndex >= 0) {
            _stack[_topIndex]->setDirty(); // Redraw the underlying screen
        }
    }
}

void ScreenManager::popToRoot() {
    while (_topIndex > 0) {
        _stack[_topIndex]->onExit();
        _stack[_topIndex] = nullptr;
        _topIndex--;
    }
    // Now _topIndex is 0 (or -1 if empty).
    // If it's 0, make sure it redraws.
    if (_topIndex == 0 && _stack[0] != nullptr) {
        _stack[0]->setDirty();
    }
}

void ScreenManager::handleInput(InputEvent event) {
    if (_topIndex >= 0 && _stack[_topIndex] != nullptr) {
        _stack[_topIndex]->handleInput(event);
    }
}

void ScreenManager::tick(U8G2& display) {
    if (_topIndex >= 0 && _stack[_topIndex] != nullptr) {
        Screen* currentScreen = _stack[_topIndex];

        // 1. Logical update
        currentScreen->tick();

        // 2. Dirty Flag Render Cascade
        if (currentScreen->needsRedraw()) {
            // Note: Screens are responsible for clearBuffer() if needed (e.g. on full redraw)
            currentScreen->draw(display);
            display.sendBuffer();
            currentScreen->clearDirty();
        }
    }
}
