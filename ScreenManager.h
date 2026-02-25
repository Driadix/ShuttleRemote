#pragma once
#include <U8g2lib.h>
#include "Screen.h"
#include "InputEvents.h"

class ScreenManager {
public:
    static ScreenManager& getInstance();

    // Navigation Stack Operations
    void push(Screen* s);
    void pop();
    void popToRoot();

    // Input Handling
    void handleInput(InputEvent event);

    // Main Loop Tick (Render Cascade)
    void tick(U8G2& display);

private:
    ScreenManager(); // Private constructor for Singleton
    ScreenManager(const ScreenManager&) = delete;
    ScreenManager& operator=(const ScreenManager&) = delete;

    static const int MAX_STACK_SIZE = 5;
    Screen* _stack[MAX_STACK_SIZE];
    int8_t _topIndex;
};
