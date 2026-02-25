#pragma once
#include <U8g2lib.h>
#include "InputEvents.h"

// Abstract base class for all UI screens
class Screen {
public:
    virtual ~Screen() {}

    // Fired when the screen is pushed to the top of the stack
    virtual void onEnter() {}

    // Fired when the screen is popped from the stack
    virtual void onExit() {}

    // Renders the layout to the display buffer. Only called if needsRedraw() is true.
    virtual void draw(U8G2& display) = 0;

    // Processes logical inputs routed from the InputManager
    virtual void handleInput(InputEvent event) = 0;

    // Called at a fixed frequency (e.g. 20Hz) to evaluate logic, animations, or check DataManager flags.
    // Implementations should check DataManager dirty flags and call setDirty() if needed.
    virtual void tick() = 0;

    // Returns true if the screen needs to be redrawn
    bool needsRedraw() const {
        return _needsRedraw;
    }

    // Forces a redraw on the next tick
    void setDirty() {
        _needsRedraw = true;
    }

    // Clears the dirty flag (called by ScreenManager after a successful render)
    void clearDirty() {
        _needsRedraw = false;
    }

protected:
    bool _needsRedraw = true; // Default to true so it draws on first load
};
