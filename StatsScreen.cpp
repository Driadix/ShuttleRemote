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

    char buf[32];

    snprintf(buf, sizeof(buf), "Dist:   %-6lu m", (unsigned long)(stats.totalDist / 1000));
    display.drawStr(0, 25, buf);

    snprintf(buf, sizeof(buf), "Cycles: %-6lu", (unsigned long)stats.loadCounter);
    display.drawStr(0, 37, buf);

    snprintf(buf, sizeof(buf), "Crashes: %-5u", stats.crashCount);
    display.drawStr(0, 49, buf);

    snprintf(buf, sizeof(buf), "Uptime:  %-6lu m", (unsigned long)stats.totalUptimeMinutes);
    display.drawStr(0, 61, buf);
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
