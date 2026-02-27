#include "DebugInfoScreen.h"
#include "ScreenManager.h"
#include <cstdio>
#include <cstdlib>

DebugInfoScreen::DebugInfoScreen() : _pageIndex(0), _lastAnimTick(0), _animState(0) {}

void DebugInfoScreen::onEnter() {
    _pageIndex = 0;
    DataManager::getInstance().setPollingMode(DataManager::PollingMode::CUSTOM_DATA);
    DataManager::getInstance().invalidateSensors(); // Request fresh data
    EventBus::subscribe(this);
}

void DebugInfoScreen::onExit() {
    EventBus::unsubscribe(this);
}

void DebugInfoScreen::onEvent(SystemEvent event) {
    if (event == SystemEvent::SENSORS_UPDATED || event == SystemEvent::CONNECTION_LOST) {
        setDirty();
    }
}

bool DebugInfoScreen::hasValidData() const {
    return DataManager::getInstance().hasValidSensors();
}

void DebugInfoScreen::drawData(U8G2& display) {
    const SP::SensorPacket& sensors = DataManager::getInstance().getSensors();

    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1);

    if (_pageIndex == 0) {
        char buf[64];
        
        // Grouped together to save vertical space for Voltage & Temp
        snprintf(buf, sizeof(buf), "Канал F:%u R:%u", sensors.distanceF, sensors.distanceR);
        display.drawUTF8(2, 10, buf);

        snprintf(buf, sizeof(buf), "Палет F:%u R:%u", sensors.distancePltF, sensors.distancePltR);
        display.drawUTF8(2, 22, buf);

        snprintf(buf, sizeof(buf), "Энкодер угл: %u", sensors.angle);
        display.drawUTF8(2, 34, buf);

        // Formatted directly without floats
        const SP::TelemetryPacket& telemetry = DataManager::getInstance().getTelemetry();
        uint16_t mv = telemetry.batteryVoltage_mV;
        snprintf(buf, sizeof(buf), "АКБ: %d.%02d V", mv / 1000, (mv % 1000) / 10);
        display.drawUTF8(2, 46, buf);

        int16_t temp = sensors.temperature_dC;
        snprintf(buf, sizeof(buf), "Температура: %d.%d C", temp / 10, abs(temp % 10));
        display.drawUTF8(2, 58, buf);

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
            display.drawUTF8(2, 10 + i*10, buf);
        }
    }

    char pageBuf[8];
    snprintf(pageBuf, sizeof(pageBuf), "%d/2", _pageIndex + 1);
    display.drawUTF8(100, 60, pageBuf);
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
        DataManager::getInstance().sendRequest(SP::MSG_REQ_SENSORS);
    }

    if (millis() - _lastAnimTick > 250) {
        _lastAnimTick = millis();
        _animState = (_animState + 1) % 4;
        setDirty();
    }
}