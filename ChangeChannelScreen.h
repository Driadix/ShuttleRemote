#pragma once
#include "Screen.h"
#include "NumericSpinnerWidget.h"
#include "DataManager.h"

class ChangeChannelScreen : public Screen {
public:
    ChangeChannelScreen();

    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
    virtual void onEnter() override;

private:
    NumericSpinnerWidget _spinner;
};
