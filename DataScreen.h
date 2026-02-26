#pragma once
#include "Screen.h"
#include "DataManager.h"

// Base class for screens that require valid data payloads from the shuttle
class DataScreen : public Screen {
public:
    virtual void draw(U8G2& display) override final {
        if (!DataManager::getInstance().isConnected()) {
            display.setFont(u8g2_font_6x13_t_cyrillic);
            display.setDrawColor(1);
            display.drawUTF8(10, 30, "ОШИБКА: НЕТ СВЯЗИ");
            return;
        }

        if (!hasValidData()) {
            display.setFont(u8g2_font_6x13_t_cyrillic);
            display.setDrawColor(1);
            display.drawUTF8(20, 30, "Запрос данных...");
            return;
        }

        drawData(display);
    }

protected:
    virtual bool hasValidData() const = 0;
    virtual void drawData(U8G2& display) = 0;
};