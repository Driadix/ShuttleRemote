#pragma once
#include <U8g2lib.h>
#include "InputEvents.h"

// Abstract base class for all UI widgets
class Widget {
public:
    virtual ~Widget() {}

    // Renders the widget to the display buffer.
    // x, y: Top-left coordinates (usually relative to the screen or container)
    virtual void draw(U8G2& display, uint8_t x, uint8_t y) = 0;

    // Processes logical inputs routed from the parent Screen.
    // Returns true if the widget state changed (requiring a redraw).
    virtual bool handleInput(InputEvent event) {
        return false;
    }
};
