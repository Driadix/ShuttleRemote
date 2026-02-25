#include "StatusBarWidget.h"
#include <Arduino.h>

void StatusBarWidget::draw(U8G2& display, uint8_t x, uint8_t y) {
    const auto& telemetry = DataManager::getInstance().getTelemetry();

    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1);

    // 1. Shuttle ID (Top Left)
    // Legacy logic: 1-9 -> A1-I9, 10+ -> 10, 11...
    char idBuf[8];
    uint8_t num = telemetry.shuttleNumber;
    if (num == 0) num = 1; // Safety

    if (num <= 9) {
        snprintf(idBuf, sizeof(idBuf), "%c%d", 'A' + num - 1, num);
    } else {
        snprintf(idBuf, sizeof(idBuf), "%d", num);
    }

    display.setCursor(x, y + 10); // Baseline at y+10? passed y is usually top-left of widget area.
                                  // If widget is placed at 0,0. y=0.
                                  // Legacy used setCursor(0, 10). So y+10.
    display.print(idBuf);

    // 2. FIFO/LIFO (Center-ish)
    // Legacy used 75, 10.
    // We are relative to x, y.
    // If x=0, 75 is 75.
    display.setCursor(x + 75, y + 10);
    if (telemetry.stateFlags & 0x20) {
        display.print("LIFO");
    } else {
        display.print("FIFO");
    }

    // 3. Battery Icon (Top Right) with Toggle (Remote vs Shuttle)
    // Cycle every 3 seconds
    bool showRemote = (millis() / 3000) % 2 == 0;

    int percent;
    char label;

    if (showRemote) {
        percent = DataManager::getInstance().getRemoteBatteryLevel();
        label = 'P'; // Pult (Remote)
    } else {
        percent = DataManager::getInstance().getTelemetry().batteryCharge;
        label = 'S'; // Shuttle
    }

    // Draw Label
    char labelBuf[2] = {label, '\0'};
    display.setCursor(x + 98, y + 10);
    display.print(labelBuf);

    // Draw Icon
    uint8_t width = 0;
    if (percent > 95) width = 14;
    else if (percent > 75) width = 11;
    else if (percent > 50) width = 8;
    else if (percent > 25) width = 5;
    else if (percent > 7) width = 2;
    else width = 0;

    display.drawFrame(x + 107, y + 1, 18, 10);
    display.drawBox(x + 125, y + 4, 2, 4); // Tip

    if (width > 0) {
        display.drawBox(x + 109, y + 3, width, 6);
    }
}
