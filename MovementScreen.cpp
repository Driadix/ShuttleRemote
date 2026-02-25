#include "MovementScreen.h"
#include "UI_Graph.h" // For instances
#include "ScreenManager.h"
#include <cstdio>

// --- MovementScreen ---

const char* MovementScreen::_movItems[3] = {
    "Movement >>",
    "Movement <<",
    "Back"
};

MovementScreen::MovementScreen()
    : _statusBar(),
      _menuList(_movItems, 3, 4)
{
}

void MovementScreen::onEnter() {
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
    if (DataManager::getInstance().consumeTelemetryDirtyFlag()) {
        setDirty();
    }
}


// --- MovementAxisScreen ---

const char* MovementAxisScreen::_axisItems[9] = {
    "10 cm", "20 cm", "30 cm", "50 cm", "100 cm", "200 cm", "300 cm", "500 cm", "Back"
};

const int32_t MovementAxisScreen::_distances[8] = {
    100, 200, 300, 500, 1000, 2000, 3000, 5000 // millimeters
};

MovementAxisScreen::MovementAxisScreen()
    : _statusBar(),
      _menuList(_axisItems, 9, 4),
      _isForward(true)
{
}

void MovementAxisScreen::setDirection(bool forward) {
    _isForward = forward;
}

void MovementAxisScreen::onEnter() {
}

void MovementAxisScreen::draw(U8G2& display) {
    _statusBar.draw(display, 0, 0);

    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setCursor(0, 15); // Wait, StatusBar is at 0, draws approx 10px high?
    // StatusBarWidget implementation check: draws at y. Usually takes some space.
    // Assuming it's top bar.

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
    if (DataManager::getInstance().consumeTelemetryDirtyFlag()) {
        setDirty();
    }
}
