#pragma once
#include <U8g2lib.h>
#include "InputEvents.h"
#include "EventBus.h"

// Abstract base class for all UI screens
class Screen : public EventListener {
public:
    virtual ~Screen() {}

    // Fired when the screen is pushed to the top of the stack
    virtual void onEnter() {
        _fullRedrawNeeded = true;
    }

    // Fired when the screen is popped from the stack
    virtual void onExit() {}

    // Renders the layout to the display buffer. Only called if needsRedraw() is true.
    // NOTE: ScreenManager no longer clears the buffer automatically.
    // If _fullRedrawNeeded is true, the implementation MUST clear the buffer (display.clearBuffer()).
    virtual void draw(U8G2& display) = 0;

    // Processes logical inputs routed from the InputManager
    virtual void handleInput(InputEvent event) = 0;

    // Called at a fixed frequency (e.g. 20Hz) to evaluate logic, animations, or check DataManager flags.
    // Implementations should check DataManager dirty flags (legacy) or handle events.
    virtual void tick() = 0;

    // Event Listener Interface
    virtual void onEvent(SystemEvent event) override {} // Default empty implementation

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
        _fullRedrawNeeded = false;
    }

protected:
    bool _needsRedraw = true; // Default to true so it draws on first load
    bool _fullRedrawNeeded = true; // Signals that a full clear/redraw is required
};
