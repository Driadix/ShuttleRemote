#pragma once
#include "Screen.h"
#include "ScrollingListWidget.h"
#include "DataManager.h"
#include "EventBus.h"

class ConfigEditorScreen : public Screen {
public:
    ConfigEditorScreen();

    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
    virtual void onEnter() override;
    virtual void onExit() override;
    virtual void onEvent(SystemEvent event) override;

    static SP::FullConfigPacket _localConfig; // Single static instance for local RAM edits

private:
    ScrollingListWidget _menuList;
    bool _isLoading;
    uint32_t _requestTimer;

    static const int ITEM_COUNT = 13;
    static void provideMenuItem(uint8_t index, char* buffer);
    void adjustValue(int idx, bool increase);
};