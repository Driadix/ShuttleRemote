#pragma once
#include "Widget.h"
#include "DataManager.h"

// Persistent banner displaying Battery %, FIFO/LIFO mode, and Shuttle ID.
// Pure observer widget (no input handling).
class StatusBarWidget : public Widget {
public:
    StatusBarWidget() : Widget(128, 14) {} // Set size

    virtual void draw(U8G2& display, uint8_t x, uint8_t y) override;

    // Override handleInput if needed, but usually status bar doesn't take input.
    // virtual bool handleInput(InputEvent event) override { return false; }
};
