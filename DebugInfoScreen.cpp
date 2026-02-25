#include "DebugInfoScreen.h"
#include "ScreenManager.h"
#include <cstdio>

DebugInfoScreen::DebugInfoScreen() : _pageIndex(0), _lastAnimTick(0), _animState(0) {}

void DebugInfoScreen::onEnter() {
    _pageIndex = 0;
    DataManager::getInstance().setPollingMode(DataManager::PollingMode::CUSTOM_DATA);
    EventBus::subscribe(this);
}

void DebugInfoScreen::onExit() {
    EventBus::unsubscribe(this);
}

void DebugInfoScreen::onEvent(SystemEvent event) {
    if (event == SystemEvent::SENSORS_UPDATED) {
        setDirty();
    }
}

void DebugInfoScreen::draw(U8G2& display) {
    const SP::SensorPacket& sensors = DataManager::getInstance().getSensors();

    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1);

    // Everything is shifted up since the status bar is gone
    if (_pageIndex == 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Канал впред: %u", sensors.distanceF);
        display.drawStr(2, 10, buf);

        snprintf(buf, sizeof(buf), "Канал назад: %u", sensors.distanceR);
        display.drawStr(2, 22, buf);

        snprintf(buf, sizeof(buf), "Паллет впред: %u", sensors.distancePltF);
        display.drawStr(2, 34, buf);

        snprintf(buf, sizeof(buf), "Паллет назад: %u", sensors.distancePltR);
        display.drawStr(2, 46, buf);

        snprintf(buf, sizeof(buf), "Энкодер угл: %u", sensors.angle);
        display.drawStr(2, 58, buf);

        // Simple activity indicator at the top right
        display.setCursor(120, 10);
        if (_animState == 0) display.print("-");
        else if (_animState == 1) display.print("\\");
        else if (_animState == 2) display.print("|");
        else display.print("/");
    } else {
        for(int i=0; i<8; i++) {
            bool state = (sensors.hardwareFlags & (1 << i)) != 0;
            char buf[64];
            if (i == 0) snprintf(buf, sizeof(buf), "%d DATCHIK_F1", state);
            else snprintf(buf, sizeof(buf), "%d SENSOR_%d", state, i);
            display.drawStr(2, 10 + i*10, buf); // Shifted up
        }
    }

    // Page indicator
    char pageBuf[8];
    snprintf(pageBuf, sizeof(pageBuf), "%d/2", _pageIndex + 1);
    display.drawStr(100, 60, pageBuf);
}

void DebugInfoScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop();
        return;
    }

    if (event == InputEvent::UP_PRESS) {
        if (_pageIndex > 0) _pageIndex--;
        setDirty();
    }
    if (event == InputEvent::DOWN_PRESS) {
        if (_pageIndex < 1) _pageIndex++;
        setDirty();
    }
}

void DebugInfoScreen::tick() {
    static uint32_t lastPoll = 0;
    if (millis() - lastPoll > 1000) {
        lastPoll = millis();
        DataManager::getInstance().sendRequest(SP::MSG_REQ_SENSORS, 0);
    }

    if (millis() - _lastAnimTick > 250) {
        _lastAnimTick = millis();
        _animState = (_animState + 1) % 4;
        setDirty();
    }
}