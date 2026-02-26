#pragma once
#include "Screen.h"
#include "ScrollingListWidget.h"
#include "DataManager.h"

class EngineerMenuScreen : public Screen {
public:
    EngineerMenuScreen();

    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
    virtual void onEnter() override;

private:
    ScrollingListWidget _menuList;
    static const int ITEM_COUNT = 7;
    static void provideMenuItem(uint8_t index, char* buffer);
};