#pragma once
#include "Screen.h"
#include "StatusBarWidget.h"
#include "ScrollingListWidget.h"
#include "DataManager.h"
#include "EventBus.h"

class OptionsScreen : public Screen {
public:
    OptionsScreen();

    // Screen Lifecycle
    virtual void onEnter() override;
    virtual void onExit() override;

    // Rendering & Input
    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;

    // Observer
    virtual void onEvent(SystemEvent event) override;

private:
    StatusBarWidget _statusBar;
    ScrollingListWidget _menuList; // We need to update this to point to UIBuffer

    static const int OPT_ITEM_COUNT = 8;
    static void provideMenuItem(uint8_t index, char* buffer);

    void adjustValue(int idx, bool increase);
};
