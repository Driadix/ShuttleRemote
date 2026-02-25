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
    if (telemetry.batteryCharge > 100) {
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

    // 4. Remote Controller Battery Icon (Animated if charging)
    int percent = DataManager::getInstance().getRemoteBatteryLevel();
    
    uint8_t width = 0;
    if (percent > 95) width = 14;
    else if (percent > 75) width = 11;
    else if (percent > 50) width = 8;
    else if (percent > 25) width = 5;
    else if (percent > 7)  width = 2;
    
    // Draw Battery Outline
    display.drawFrame(x + 107, y + 1, 18, 10);
    display.drawBox(x + 125, y + 4, 2, 4); // Tip
    
    // Fill Battery Level
    if (width > 0) {
        display.drawBox(x + 109, y + 3, width, 6);
    }
}