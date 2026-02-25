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

    // 3. Battery Icon (Top Right)
    // Legacy used 107, 1.
    // Ported Logic from BatteryLevel(uint8_t percent)
    // We use telemetry.batteryCharge
    uint8_t percent = telemetry.batteryCharge;

    // Smoothness logic from legacy (prevpercent) omitted as Widget is redrawn fresh.
    // But we need persistent state for smoothness?
    // The legacy code used static/global prevpercent.
    // "if (percent < prevpercent || percent > prevpercent + 5) prevpercent = percent;"
    // This is hysteresis.
    // For a widget, we might want to store this state if strict adherence is needed.
    // But for now, direct mapping is fine.

    uint8_t width = 0;
    if (percent > 95) width = 14;
    else if (percent > 75) width = 11;
    else if (percent > 50) width = 8;
    else if (percent > 25) width = 5;
    else if (percent > 7) width = 2;
    else width = 0;

    // Legacy: drawFrame(107, 1, 18, 10);
    // Relative to x, y.
    display.drawFrame(x + 107, y + 1, 18, 10);
    display.drawBox(x + 125, y + 4, 2, 4); // Tip

    if (width > 0) {
        display.drawBox(x + 109, y + 3, width, 6);
    }
}
