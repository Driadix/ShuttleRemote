#pragma once
#include "Widget.h"
#include "DataManager.h"

// Persistent banner displaying Battery %, FIFO/LIFO mode.
class StatusBarWidget : public Widget {
public:
    StatusBarWidget() : Widget(128, 14) {}

    virtual void draw(U8G2& display, uint8_t x, uint8_t y) override;
};