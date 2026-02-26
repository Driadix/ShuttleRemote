#pragma once
#include "Screen.h"

class RemoteSettingsScreen : public Screen {
public:
    RemoteSettingsScreen();
    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;
};