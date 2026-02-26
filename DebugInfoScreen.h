#pragma once
#include "DataScreen.h"
#include "DataManager.h"
#include "EventBus.h"

class DebugInfoScreen : public DataScreen {
public:
    DebugInfoScreen();

    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
    virtual void onEnter() override;
    virtual void onExit() override;
    virtual void onEvent(SystemEvent event) override;

protected:
    virtual bool hasValidData() const override;
    virtual void drawData(U8G2& display) override;

private:
    uint8_t _pageIndex; // 0 = Sensors, 1 = Flags
    uint32_t _lastAnimTick;
    uint8_t _animState;
};