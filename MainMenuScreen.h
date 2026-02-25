#pragma once
#include "Screen.h"
#include "StatusBarWidget.h"
#include "ScrollingListWidget.h"
#include "DataManager.h"

class MainMenuScreen : public Screen {
public:
    MainMenuScreen();

    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
    virtual void onEnter() override;

private:
    StatusBarWidget _statusBar;
    ScrollingListWidget _menuList;

    // Dynamic Menu Items Buffer
    // We need buffers for items that change (Pallet Count, Mode)
    // Legacy had 8 items. We add Engineering Menu -> 9 items.
    static const int MENU_ITEM_COUNT = 9;
    static char _menuItemBuffers[MENU_ITEM_COUNT][32];
    static const char* _menuItemsPtrs[MENU_ITEM_COUNT];

    void updateMenuItems();
};
