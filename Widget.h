#pragma once
#include <U8g2lib.h>
#include "InputEvents.h"

// Abstract base class for all UI widgets
class Widget {
public:
    Widget(uint8_t w = 0, uint8_t h = 0) : _width(w), _height(h), _needsRedraw(true) {}
    virtual ~Widget() {}

    // Renders the widget to the display buffer.
    // x, y: Top-left coordinates (usually relative to the screen or container)
    // Implementation should clear its background (drawBox with color 0) if it expects to be drawn over existing content.
    virtual void draw(U8G2& display, uint8_t x, uint8_t y) = 0;

    // Processes logical inputs routed from the parent Screen.
    // Returns true if the widget state changed (requiring a redraw).
    virtual bool handleInput(InputEvent event) {
        return false;
    }

    uint8_t getWidth() const { return _width; }
    uint8_t getHeight() const { return _height; }

    void setSize(uint8_t w, uint8_t h) {
        _width = w;
        _height = h;
    }

    bool needsRedraw() const { return _needsRedraw; }
    void setDirty() { _needsRedraw = true; }
    void clearDirty() { _needsRedraw = false; }

protected:
    uint8_t _width;
    uint8_t _height;
    bool _needsRedraw;
};
