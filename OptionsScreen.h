#pragma once
#include "Screen.h"
#include "ScrollingListWidget.h"
#include "DataManager.h"
#include "EventBus.h"

class OptionsScreen : public Screen {
public:
    OptionsScreen();

    virtual void onEnter() override;
    virtual void onExit() override;
    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
    virtual void onEvent(SystemEvent event) override;

private:
    ScrollingListWidget _menuList;
    static const int OPT_ITEM_COUNT = 8;
    static void provideMenuItem(uint8_t index, char* buffer);
    void adjustValue(int idx, bool increase);
};