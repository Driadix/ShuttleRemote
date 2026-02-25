#pragma once
#include "Screen.h"
#include "StatusBarWidget.h"
#include "ScrollingListWidget.h"
#include "DataManager.h"

class EngineeringMenuScreen : public Screen {
public:
    EngineeringMenuScreen();

    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
    virtual void onEnter() override;

private:
    StatusBarWidget _statusBar;
    ScrollingListWidget _menuList;

    static const int ENG_ITEM_COUNT = 12;
    static char _engItemBuffers[ENG_ITEM_COUNT][32];
    static const char* _engItemsPtrs[ENG_ITEM_COUNT];

    void updateMenuItems();
    void adjustValue(int idx, bool increase);
};
