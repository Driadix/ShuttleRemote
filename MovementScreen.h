#pragma once
#include "Screen.h"
#include "StatusBarWidget.h"
#include "ScrollingListWidget.h"
#include "DataManager.h"

class MovementScreen : public Screen {
public:
    MovementScreen();

    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
    virtual void onEnter() override;

private:
    StatusBarWidget _statusBar;
    ScrollingListWidget _menuList;

    static const int MOV_ITEM_COUNT = 3;
    static const char* _movItems[MOV_ITEM_COUNT];
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

private:
    StatusBarWidget _statusBar;
    ScrollingListWidget _menuList;

    static const int AXIS_ITEM_COUNT = 9;
    static const char* _axisItems[AXIS_ITEM_COUNT];
    static const int32_t _distances[8]; // Values for items

    bool _isForward;
};
