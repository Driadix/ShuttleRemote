#include "MovementScreen.h"
#include "UI_Graph.h" // For instances
#include "ScreenManager.h"
#include <cstdio>

// --- MovementScreen ---

MovementScreen::MovementScreen()
    : _statusBar(),
      _menuList(MovementScreen::provideMenuItem, 3, 4)
{
}

void MovementScreen::provideMenuItem(uint8_t index, char* buffer) {
    switch(index) {
        case 0: strcpy(buffer, "Movement >>"); break;
        case 1: strcpy(buffer, "Movement <<"); break;
        case 2: strcpy(buffer, "Back"); break;
        default: buffer[0] = '\0'; break;
    }
}

void MovementScreen::onEnter() {
    EventBus::subscribe(this);
}

void MovementScreen::onExit() {
    EventBus::unsubscribe(this);
}

void MovementScreen::onEvent(SystemEvent event) {
    if (event == SystemEvent::TELEMETRY_UPDATED) {
        setDirty();
    }
}

void MovementScreen::draw(U8G2& display) {
    _statusBar.draw(display, 0, 0);
    _menuList.draw(display, 0, 16);
}

void MovementScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop();
        return;
    }

    if (event == InputEvent::OK_SHORT_PRESS) {
        int idx = _menuList.getCursorIndex();
        switch(idx) {
            case 0: // >>
                movementAxisScreen.setDirection(true);
                ScreenManager::getInstance().push(&movementAxisScreen);
                break;
            case 1: // <<
                movementAxisScreen.setDirection(false);
                ScreenManager::getInstance().push(&movementAxisScreen);
                break;
            case 2: // Back
                ScreenManager::getInstance().pop();
                break;
        }
        return;
    }

    if (_menuList.handleInput(event)) {
        setDirty();
    }
}

void MovementScreen::tick() {
}


// --- MovementAxisScreen ---

const int32_t MovementAxisScreen::_distances[8] = {
    100, 200, 300, 500, 1000, 2000, 3000, 5000 // millimeters
};

MovementAxisScreen::MovementAxisScreen()
    : _statusBar(),
      _menuList(MovementAxisScreen::provideMenuItem, 9, 4),
      _isForward(true)
{
}

void MovementAxisScreen::provideMenuItem(uint8_t index, char* buffer) {
    if (index < 8) {
        snprintf(buffer, 32, "%ld cm", (long)_distances[index] / 10);
    } else {
        strcpy(buffer, "Back");
    }
}

void MovementAxisScreen::setDirection(bool forward) {
    _isForward = forward;
}

void MovementAxisScreen::onEnter() {
    EventBus::subscribe(this);
}

void MovementAxisScreen::onExit() {
    EventBus::unsubscribe(this);
}

void MovementAxisScreen::onEvent(SystemEvent event) {
    if (event == SystemEvent::TELEMETRY_UPDATED) {
        setDirty();
    }
}

void MovementAxisScreen::draw(U8G2& display) {
    _statusBar.draw(display, 0, 0);

    display.setFont(u8g2_font_6x13_t_cyrillic);
    // Show direction indicator
    display.setCursor(110, 25);
    display.print(_isForward ? ">>" : "<<");

    _menuList.draw(display, 0, 16);
}

void MovementAxisScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop();
        return;
    }

    if (event == InputEvent::OK_SHORT_PRESS) {
        int idx = _menuList.getCursorIndex();

        if (idx < 8) {
            // Send command
            SP::CmdType cmd = _isForward ? SP::CMD_MOVE_DIST_F : SP::CMD_MOVE_DIST_R;
            int32_t dist = _distances[idx];

            if (DataManager::getInstance().sendCommand(cmd, dist)) {
                // Success feedback? Maybe pop back to MovementScreen?
                ScreenManager::getInstance().pop();
            } else {
                // Queue full
                // Could show warning, but ScreenManager handles it?
                // No, DashboardScreen handles it.
                // We should probably stay here.
            }
        } else {
            // Back
            ScreenManager::getInstance().pop();
        }
        return;
    }

    if (_menuList.handleInput(event)) {
        setDirty();
    }
}

void MovementAxisScreen::tick() {
}
