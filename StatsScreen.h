#pragma once
#include "Screen.h"
#include "StatusBarWidget.h"
#include "DataManager.h"

class StatsScreen : public Screen {
public:
    StatsScreen();

    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;

    virtual void onEnter() override; // To trigger poll context

private:
    StatusBarWidget _statusBar;
};
