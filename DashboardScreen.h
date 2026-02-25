#pragma once
#include "Screen.h"
#include "StatusBarWidget.h"
#include "DataManager.h"
#include "ScreenManager.h"

class DashboardScreen : public Screen {
public:
    DashboardScreen();

    virtual void draw(U8G2& display) override;
    virtual void handleInput(InputEvent event) override;
    virtual void tick() override;

private:
    StatusBarWidget _statusBar;

    // Helper to draw the animated shuttle status
    void drawShuttleStatus(U8G2& display, const SP::TelemetryPacket& tel);

    // Helper to draw warnings
    void drawWarnings(U8G2& display, const SP::TelemetryPacket& tel);

    // Manual mode state
    bool _isManualMoving;
    String _manualCommand;
    uint32_t _queueFullTimer;
    bool _showQueueFull;
};
