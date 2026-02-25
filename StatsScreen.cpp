#include "StatsScreen.h"
#include "ScreenManager.h"

StatsScreen::StatsScreen() {}

void StatsScreen::onEnter() {
    DataManager::getInstance().setPollContext(DataManager::PollContext::STATS_VIEW);
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
    _statusBar.draw(display, 0, 0);

    const auto& stats = DataManager::getInstance().getStats();

    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1);

    display.setCursor(0, 25);
    display.print("Dist: " + String(stats.totalDist / 1000) + "m");

    display.setCursor(0, 37);
    display.print("Cycles: " + String(stats.loadCounter));

    display.setCursor(0, 49);
    display.print("Crashes: " + String(stats.crashCount));

    display.setCursor(0, 61);
    display.print("Uptime: " + String(stats.totalUptimeMinutes) + "m");
}

void StatsScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop();
        return;
    }
}

void StatsScreen::tick() {
    // Keep context active
    DataManager::getInstance().setPollContext(DataManager::PollContext::STATS_VIEW);
}
