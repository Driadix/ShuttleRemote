#pragma once
#include "Screen.h"
#include "ScrollingListWidget.h"
#include "DataManager.h"
#include "EventBus.h"

class MovementScreen : public Screen {
public:
    MovementScreen();

    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
    virtual void onEnter() override;
    virtual void onExit() override;
    virtual void onEvent(SystemEvent event) override;

private:
    ScrollingListWidget _menuList;

    static const int MOV_ITEM_COUNT = 3;
    static void provideMenuItem(uint8_t index, char* buffer);
};

class MovementAxisScreen : public Screen {
public:
    MovementAxisScreen();

    // Set direction: true = Forward (>>), false = Reverse (<<)
    void setDirection(bool forward);

    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
    virtual void onEnter() override;
    virtual void onExit() override;
    virtual void onEvent(SystemEvent event) override;

private:
    ScrollingListWidget _menuList;

    static const int AXIS_ITEM_COUNT = 9;
    static void provideMenuItem(uint8_t index, char* buffer);
    static const int32_t _distances[8]; // Values for items

    bool _isForward;
};