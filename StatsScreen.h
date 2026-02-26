#pragma once
#include "DataScreen.h"
#include "DataManager.h"
#include "EventBus.h"

class StatsScreen : public DataScreen {
public:
    StatsScreen();

    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;

    virtual void onEnter() override;
    virtual void onExit() override;
    virtual void onEvent(SystemEvent event) override;

protected:
    virtual bool hasValidData() const override;
    virtual void drawData(U8G2& display) override;
};