#pragma once
#include "Widget.h"
#include "DataManager.h"

// Persistent banner displaying Battery %, FIFO/LIFO mode.
class StatusBarWidget : public Widget {
public:
    StatusBarWidget() : Widget(128, 14), _lastChargeAnimTick(0), _chargeAnimFrame(0) {}

    virtual void draw(U8G2& display, uint8_t x, uint8_t y) override;
    virtual void tick() override;

private:
    uint32_t _lastChargeAnimTick;
    uint8_t _chargeAnimFrame;
};