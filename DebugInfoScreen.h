#pragma once
#include "Screen.h"
#include "StatusBarWidget.h"
#include "DataManager.h"

class DebugInfoScreen : public Screen {
public:
    DebugInfoScreen();

    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
    virtual void onEnter() override;

private:
    StatusBarWidget _statusBar;
    uint8_t _pageIndex; // 0 = Sensors, 1 = Flags
};
