#pragma once
#include "Screen.h"
#include "NumericSpinnerWidget.h"
#include "ScreenManager.h"
#include "DataManager.h"

class UnloadPalletsScreen : public Screen {
public:
    UnloadPalletsScreen();

    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
    virtual void onEnter() override;

private:
    NumericSpinnerWidget _spinner;

    enum State {
        ENTRY,
        SUCCESS,
        FAIL
    };
    State _state;
    uint32_t _stateTimer;
    int _quantity;
};
