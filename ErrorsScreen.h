#pragma once
#include "Screen.h"
#include "StatusBarWidget.h"
#include "ScrollingListWidget.h"
#include "DataManager.h"
#include "EventBus.h"

class ErrorsScreen : public Screen {
public:
    ErrorsScreen();

    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
    virtual void onEnter() override;
    virtual void onExit() override;
    virtual void onEvent(SystemEvent event) override;

private:
    StatusBarWidget _statusBar;
    ScrollingListWidget _menuList;

    // Max 16 errors + "Reset" + "Back" = 18 items max
    static const int MAX_ERR_ITEMS = 18;
    static uint8_t _activeErrorCodes[MAX_ERR_ITEMS];
    static uint8_t _totalItems;

    void updateErrorList();
    static void provideErrorItem(uint8_t index, char* buffer);
};
