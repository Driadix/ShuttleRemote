#pragma once
#include "Screen.h"
#include "StatusBarWidget.h"
#include "ScrollingListWidget.h"
#include "DataManager.h"

class OptionsScreen : public Screen {
public:
    OptionsScreen();

    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
    virtual void onEnter() override;

private:
    StatusBarWidget _statusBar;
    ScrollingListWidget _menuList;

    static const int OPT_ITEM_COUNT = 8;
    static char _optItemBuffers[OPT_ITEM_COUNT][32];
    static const char* _optItemsPtrs[OPT_ITEM_COUNT];

    void updateMenuItems();
    void adjustValue(int idx, bool increase);
};
