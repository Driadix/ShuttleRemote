#include "MainMenuScreen.h"
#include "UI_Graph.h" 
#include "ScreenManager.h"
#include <cstdio> 

MainMenuScreen::MainMenuScreen()
    : _menuList(MainMenuScreen::provideMenuItem, MENU_ITEM_COUNT, 4) {}

void MainMenuScreen::onEnter() { EventBus::subscribe(this); }
void MainMenuScreen::onExit() { EventBus::unsubscribe(this); }

void MainMenuScreen::onEvent(SystemEvent event) {
    if (event == SystemEvent::TELEMETRY_UPDATED || event == SystemEvent::CONFIG_UPDATED) {
        setDirty();
    }
}

void MainMenuScreen::provideMenuItem(uint8_t index, char* buffer) {
    switch(index) {
        case 0: strcpy(buffer, "Настройки"); break;
        case 1: strcpy(buffer, "Ошибки"); break;
        case 2: strcpy(buffer, "Выгрузить паллеты"); break;
        case 3: strcpy(buffer, "Инженерное меню"); break;
        case 4: snprintf(buffer, 64, "Счет паллет: %d", DataManager::getInstance().getTelemetry().palleteCount); break;
        case 5: strcpy(buffer, "Уплотнение"); break;
        case 6: {
            bool fifoLifo = (DataManager::getInstance().getTelemetry().stateFlags & 0x20) != 0;
            snprintf(buffer, 64, "Режим: %s", fifoLifo ? "LIFO" : "FIFO");
            break;
        }
        case 7: strcpy(buffer, "Возврат домой"); break;
        case 8: strcpy(buffer, "Назад"); break;
        default: buffer[0] = '\0'; break;
    }
}

void MainMenuScreen::draw(U8G2& display) {
    _menuList.draw(display, 0, 0); // Drawn completely at the top
}

void MainMenuScreen::handleInput(InputEvent event) {
    if (event == InputEvent::BACK_PRESS) {
        ScreenManager::getInstance().pop(); 
        return;
    }

    if (event == InputEvent::OK_SHORT_PRESS) {
        int idx = _menuList.getCursorIndex();
        switch(idx) {
            case 0: ScreenManager::getInstance().push(&optionsScreen); break;
            case 1: ScreenManager::getInstance().push(&errorsScreen); break;
            case 2: ScreenManager::getInstance().push(&unloadPalletsScreen); break;
            case 3: 
                pinEntryScreen.setTarget(&engineeringMenuScreen);
                ScreenManager::getInstance().push(&pinEntryScreen);
                break;
            case 4: DataManager::getInstance().sendCommand(SP::CMD_COUNT_PALLETS); break;
            case 5: DataManager::getInstance().sendCommand(SP::CMD_COMPACT_F); break;
            case 6: {
                bool newMode = !((DataManager::getInstance().getTelemetry().stateFlags & 0x20) != 0);
                DataManager::getInstance().setConfig(SP::CFG_FIFO_LIFO, newMode ? 1 : 0);
                setDirty();
                break;
            }
            case 7: DataManager::getInstance().sendCommand(SP::CMD_HOME); break;
            case 8: ScreenManager::getInstance().pop(); break;
        }
        return;
    }

    if (_menuList.handleInput(event)) {
        setDirty();
    }
}

void MainMenuScreen::tick() {}