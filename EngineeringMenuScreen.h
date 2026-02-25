#pragma once
#include "Screen.h"
#include "StatusBarWidget.h"
#include "ScrollingListWidget.h"
#include "DataManager.h"
#include "EventBus.h"

class EngineeringMenuScreen : public Screen, public EventListener {
public:
    EngineeringMenuScreen();

    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
    virtual void onEnter() override;
    virtual void onExit() override;
    virtual void onEvent(SystemEvent event) override;

private:
    StatusBarWidget _statusBar;
    ScrollingListWidget _menuList;

    static const int ENG_ITEM_COUNT = 12;
    static void provideMenuItem(uint8_t index, char* buffer);

    void adjustValue(int idx, bool increase);
};
