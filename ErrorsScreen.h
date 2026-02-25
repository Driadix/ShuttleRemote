#pragma once
#include "Screen.h"
#include "StatusBarWidget.h"
#include "ScrollingListWidget.h"
#include "DataManager.h"

class ErrorsScreen : public Screen {
public:
    ErrorsScreen();

    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
    virtual void onEnter() override;

private:
    StatusBarWidget _statusBar;
    ScrollingListWidget _menuList;

    // Max 16 errors + "Reset" + "Back" = 18 items max
    static const int MAX_ERR_ITEMS = 18;
    static char _errItemBuffers[MAX_ERR_ITEMS][32];
    static const char* _errItemsPtrs[MAX_ERR_ITEMS];

    int _activeErrorCount;

    void updateErrorList();
};
