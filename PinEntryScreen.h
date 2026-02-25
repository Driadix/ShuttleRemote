#pragma once
#include "Screen.h"
#include "NumericSpinnerWidget.h"
#include "ScreenManager.h"

class PinEntryScreen : public Screen {
public:
    PinEntryScreen();

    void setTarget(Screen* target);

    virtual void onEnter() override;
    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;

private:
    NumericSpinnerWidget _spinner;
    Screen* _targetScreen;
    bool _showAccessDenied;
    uint32_t _accessDeniedTimer;

    const int PIN_CODE = 1441;
};
