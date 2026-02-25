#include "DebugInfoScreen.h"
#include "ScreenManager.h"

DebugInfoScreen::DebugInfoScreen() : _pageIndex(0), _lastAnimTick(0), _animState(0) {}

void DebugInfoScreen::onEnter() {
    _pageIndex = 0;
    DataManager::getInstance().setPollContext(DataManager::PollContext::DEBUG_SENSORS);
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
    _statusBar.draw(display, 0, 0);

    const SP::SensorPacket& sensors = DataManager::getInstance().getSensors();

    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1);

    if (_pageIndex == 0) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Forw. chnl dist: %u", sensors.distanceF);
        display.drawStr(2, 22, buf);

        snprintf(buf, sizeof(buf), "Rvrs. chnl dist: %u", sensors.distanceR);
        display.drawStr(2, 32, buf);

        snprintf(buf, sizeof(buf), "Forw. plt dist:  %u", sensors.distancePltF);
        display.drawStr(2, 42, buf);

        snprintf(buf, sizeof(buf), "Rvrs. plt dist:  %u", sensors.distancePltR);
        display.drawStr(2, 52, buf);

        snprintf(buf, sizeof(buf), "Encoder ang: %u", sensors.angle);
        display.drawStr(2, 62, buf);

        // Simple activity indicator
        // display.drawGlyph(120, 20, 0x25f7 - _animState); // Requires symbol font
        display.setCursor(120, 20);
        if (_animState == 0) display.print("-");
        else if (_animState == 1) display.print("\\");
        else if (_animState == 2) display.print("|");
        else display.print("/");
    } else {
        display.setFont(u8g2_font_6x13_t_cyrillic);
        for(int i=0; i<8; i++) {
            bool state = (sensors.hardwareFlags & (1 << i)) != 0;
            char buf[32];
            if (i == 0) snprintf(buf, sizeof(buf), "%d DATCHIK_F1", state);
            else snprintf(buf, sizeof(buf), "%d SENSOR_%d", state, i);
            display.drawStr(2, 20 + i*10, buf);
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

    // Toggle page with UP/DOWN or OK?
    // Legacy used UP/DOWN to change cursorPos (1 or 2).
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
    DataManager::getInstance().setPollContext(DataManager::PollContext::DEBUG_SENSORS);
    if (millis() - _lastAnimTick > 250) {
        _lastAnimTick = millis();
        _animState = (_animState + 1) % 4;
        setDirty();
    }
}
