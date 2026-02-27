#include "StatusBarWidget.h"
#include "Lang_RU.h"
#include <Arduino.h>

void StatusBarWidget::draw(U8G2& display, uint8_t x, uint8_t y) {
    const auto& telemetry = DataManager::getInstance().getTelemetry();

    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1);

    // 1. "Заряд" Text
    display.setCursor(x, y + 10);
    display.print(TXT_CHARGE);

    // 2. Battery Percentage (Shuttle)
    char battBuf[16];
    if (!DataManager::getInstance().isConnected()) {
        snprintf(battBuf, sizeof(battBuf), "[N/A]");
    } else if (telemetry.batteryCharge > 100) {
        snprintf(battBuf, sizeof(battBuf), "[N/A]");
    } else {
        snprintf(battBuf, sizeof(battBuf), "[%d%%]", telemetry.batteryCharge);
    }
    display.setCursor(x + 35, y + 10);
    display.print(battBuf);

    // 3. FIFO / LIFO
    display.setCursor(x + 75, y + 10);
    if (telemetry.stateFlags & 0x20) {
        display.print(TXT_LIFO);
    } else {
        display.print(TXT_FIFO);
    }

    // 4. Remote Controller Battery Icon 
    int percent = DataManager::getInstance().getRemoteBatteryLevel();
    bool charging = DataManager::getInstance().isCharging();
    
    uint8_t width = 0;
    if (charging) {
        // 0 -> 2, 1 -> 5, 2 -> 8, 3 -> 11, 4 -> 14
        width = 2 + (_chargeAnimFrame * 3);
        if (width > 14) width = 14;
    } else {
        if (percent > 95) width = 14;
        else if (percent > 75) width = 11;
        else if (percent > 50) width = 8;
        else if (percent > 25) width = 5;
        else if (percent > 7)  width = 2;
    }
    
    display.drawFrame(x + 107, y + 1, 18, 10);
    display.drawBox(x + 125, y + 4, 2, 4);
    
    if (width > 0) {
        display.drawBox(x + 109, y + 3, width, 6);
    }
}

void StatusBarWidget::tick() {
    bool isCharging = DataManager::getInstance().isCharging();
    int currentBattery = DataManager::getInstance().getRemoteBatteryLevel();
    
    static bool lastCharging = false;
    static int lastBattery = -1;
    
    if (isCharging != lastCharging || currentBattery != lastBattery) {
        lastCharging = isCharging;
        lastBattery = currentBattery;
        setDirty();
    }

    if (isCharging) {
        if (millis() - _lastChargeAnimTick > 500) {
            _lastChargeAnimTick = millis();
            _chargeAnimFrame++;
            if (_chargeAnimFrame > 4) _chargeAnimFrame = 0;
            setDirty();
        }
    } else {
        _chargeAnimFrame = 4;
    }
}