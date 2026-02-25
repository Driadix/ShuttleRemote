#pragma once
#include "Screen.h"
#include "StatusBarWidget.h"
#include "DataManager.h"

class DashboardScreen : public Screen {
public:
    DashboardScreen();

    // Observer Lifecycle
    void onEnter() override;
    void onExit() override;
    void onEvent(SystemEvent event) override;

    void draw(U8G2& display) override;
    void handleInput(InputEvent event) override;
    void tick() override;

private:
    void drawShuttleStatus(U8G2& display, const SP::TelemetryPacket& cachedTelemetry);

    StatusBarWidget _statusBar;
    bool _isManualMoving;
    String _manualCommand;
    uint32_t _queueFullTimer;
    bool _showQueueFull;
};
