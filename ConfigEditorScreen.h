#pragma once
#include "DataScreen.h"
#include "ScrollingListWidget.h"
#include "DataManager.h"
#include "EventBus.h"

class ConfigEditorScreen : public DataScreen {
public:
    ConfigEditorScreen();

    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
    virtual void onEnter() override;
    virtual void onExit() override;
    virtual void onEvent(SystemEvent event) override;

    static SP::FullConfigPacket _localConfig; 

protected:
    virtual bool hasValidData() const override;
    virtual void drawData(U8G2& display) override;

private:
    ScrollingListWidget _menuList;
    uint32_t _requestTimer;

    static const int ITEM_COUNT = 13;
    static void provideMenuItem(uint8_t index, char* buffer);
    void adjustValue(int idx, bool increase);
};