#pragma once
#include "Screen.h"
#include "StatusBarWidget.h"
#include "DataManager.h"

class DashboardScreen : public Screen {
public:
    DashboardScreen();

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

    uint8_t _tempShuttleNum;
    bool _isSelectingShuttle;
    bool _blinkState;
    uint32_t _shuttleSelectTimer;
    uint32_t _lastBlinkTick;

    // Animation State
    uint8_t _animX;
    uint8_t _animFlagX;
    uint32_t _lastAnimTick;

    // Action Bar State
    String _actionMsg;
    String _actionStatus;
    uint32_t _actionMsgTimer;
};