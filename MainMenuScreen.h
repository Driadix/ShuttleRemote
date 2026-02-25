#pragma once
#include "Screen.h"
#include "ScrollingListWidget.h"
#include "DataManager.h"
#include "EventBus.h"

class MainMenuScreen : public Screen {
public:
    MainMenuScreen();

    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
    virtual void onEnter() override;
    virtual void onExit() override;
    virtual void onEvent(SystemEvent event) override;

private:
    ScrollingListWidget _menuList;

    static const int MENU_ITEM_COUNT = 9;
    static void provideMenuItem(uint8_t index, char* buffer);
};