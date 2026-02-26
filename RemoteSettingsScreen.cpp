#include "RemoteSettingsScreen.h"
#include "ScreenManager.h"
#include <WiFi.h>

RemoteSettingsScreen::RemoteSettingsScreen() {}

void RemoteSettingsScreen::draw(U8G2& display) {
    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1);
    
    display.drawUTF8(0, 15, "НАСТРОЙКИ ПУЛЬТА");
    
    display.drawUTF8(0, 35, "OTA Обновление: ДОСТУПНО");
    display.drawUTF8(0, 50, "WIFI IP: 192.168.4.1");
    
    display.drawUTF8(0, 63, "< Назад (7)");
}

void RemoteSettingsScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS || event == InputEvent::OK_SHORT_PRESS) {
        ScreenManager::getInstance().pop();
    }
}

void RemoteSettingsScreen::tick() {}