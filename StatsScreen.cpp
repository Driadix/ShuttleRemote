#include "StatsScreen.h"
#include "ScreenManager.h"
#include <cstdio>

StatsScreen::StatsScreen() {}

void StatsScreen::onEnter() {
    DataManager::getInstance().setPollingMode(DataManager::PollingMode::CUSTOM_DATA);
    EventBus::subscribe(this);
}

void StatsScreen::onExit() {
    EventBus::unsubscribe(this);
}

void StatsScreen::onEvent(SystemEvent event) {
    if (event == SystemEvent::STATS_UPDATED) {
        setDirty();
    }
}

void StatsScreen::draw(U8G2& display) {
    const auto& stats = DataManager::getInstance().getStats();

    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1);

    char buf[64];

    // Shifted layout up by ~15 pixels since the status bar is gone, and added Localization
    snprintf(buf, sizeof(buf), "Путь:   %-6lu м", (unsigned long)(stats.totalDist / 1000));
    display.drawStr(0, 10, buf);

    snprintf(buf, sizeof(buf), "Циклы:  %-6lu", (unsigned long)stats.loadCounter);
    display.drawStr(0, 25, buf);

    snprintf(buf, sizeof(buf), "Аварии: %-5u", stats.crashCount);
    display.drawStr(0, 40, buf);

    snprintf(buf, sizeof(buf), "Время:  %-6lu м", (unsigned long)stats.totalUptimeMinutes);
    display.drawStr(0, 55, buf);
}

void StatsScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop();
        return;
    }
}

void StatsScreen::tick() {
    static uint32_t lastPoll = 0;
    if (millis() - lastPoll > 1000) {
        lastPoll = millis();
        DataManager::getInstance().sendRequest(SP::MSG_REQ_STATS);
    }
}