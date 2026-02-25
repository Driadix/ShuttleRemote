#include "DashboardScreen.h"
#include "UI_Graph.h" // Will contain extern declarations
#include "ScreenManager.h"

// Legacy String Arrays (Copied from PultV1_0_2.ino)
static const String shuttleStatusArray[19] = {
    "Запрос статуса", "Ручной режим",      "Загрузка",      "Выгрузка",
    "Уплотнение",     "Эвакуация",         "DEMO",          "Подсчет паллет",
    "Испытания",      "Обнаружены ошибки", "Ожидание...",   "Прод. загрузка",
    "Прод. выгрузка", "Прод. выгрузка",    "  Вперед...",   "  Назад...",
    "  Вверх...",     "  Вниз...",         "  Инициация..."
};

static const String ErrorMsgArray[21] = {
    "Нет ошибок",
    "Сенсор канала F",
    "Сенсор канала R",
    "Сенсор канала DF",
    "Сенсор канала DR",
    "Сенсор паллет F",
    "Сенсор паллет R",
    "Сенсор паллет DF",
    "Сенсор паллет DR",
    "Подъемник",
    "Привод движ.",
    "Низкий заряд",
    "Столкновение",
    "Перегрев",
    "", "", ""
};

DashboardScreen::DashboardScreen()
    : _isManualMoving(false), _manualCommand(" "), _queueFullTimer(0), _showQueueFull(false) {
}

void DashboardScreen::draw(U8G2& display) {
    // 1. Draw Status Bar
    _statusBar.draw(display, 0, 0);

    const SP::TelemetryPacket& cachedTelemetry = DataManager::getInstance().getTelemetry();

    display.setFont(u8g2_font_6x13_t_cyrillic);
    display.setDrawColor(1);

    // 2. Draw Battery and FIFO/LIFO (Already in StatusBar? No, StatusBar draws generic info, let's see StatusBarWidget later.
    // The legacy code draws battery and FIFO/LIFO in the main loop.
    // StatusBarWidget should handle this according to prompt "Include your StatusBarWidget... Call _statusBarWidget.draw() first."
    // Let's assume StatusBarWidget handles top bar.

    // 3. Draw Main Status
    display.setCursor(0, 25);
    if (cachedTelemetry.shuttleStatus == 13)
        display.print("Осталось выгрузить " + String(cachedTelemetry.palleteCount));
    else if (cachedTelemetry.shuttleStatus < 19)
        display.print(shuttleStatusArray[cachedTelemetry.shuttleStatus]);
    else
        display.print("Status: " + String(cachedTelemetry.shuttleStatus));

    // 4. Draw Warnings / Queue Full / Errors
    display.setCursor(0, 40);
    if (_showQueueFull) {
        display.print("! QUEUE FULL !");
    } else if (cachedTelemetry.errorCode) {
        if (cachedTelemetry.errorCode < 21)
             display.print("! " + ErrorMsgArray[cachedTelemetry.errorCode] + " !");
        else
             display.print("! ERR " + String(cachedTelemetry.errorCode) + " !");
    }

    // 5. Draw Manual Command
    if (_isManualMoving) {
        display.setCursor(85, 40);
        display.print(_manualCommand);
    }

    // 6. Draw Animation (Legacy Logic)
    drawShuttleStatus(display, cachedTelemetry);
}

void DashboardScreen::drawShuttleStatus(U8G2& display, const SP::TelemetryPacket& cachedTelemetry) {
    if (cachedTelemetry.shuttleStatus == 3 || cachedTelemetry.shuttleStatus == 13 || cachedTelemetry.shuttleStatus == 4 || cachedTelemetry.shuttleStatus == 6)
    {
        static uint8_t x = 0;
        static uint8_t flagx = 0;
        display.drawBox(x, 40, 28, 5);
        display.drawBox(x + 4, 45, 3, 2);
        display.drawBox(x + 21, 45, 3, 2);
        if (flagx == 0 && x < 128)
            x++;
        else
        {
            flagx = 1;
            display.drawBox(x, 34, 28, 3);
            display.drawBox(x, 37, 3, 3);
            display.drawBox(x + 13, 37, 3, 3);
            display.drawBox(x + 25, 37, 3, 3);
            x--;
        }
        if (flagx == 1 && x == 0)
        {
            flagx = 0;
        }
        // Force redraw for animation
        setDirty();
    }
    else if (cachedTelemetry.shuttleStatus == 2 || cachedTelemetry.shuttleStatus == 12)
    {
        static uint8_t x = 128;
        static uint8_t flagx = 1;
        display.drawBox(x, 40, 28, 5);
        display.drawBox(x + 4, 45, 3, 2);
        display.drawBox(x + 21, 45, 3, 2);

        if (flagx == 1 && x > 1)
            x--;
        else
        {
            flagx = 0;
            display.drawBox(x, 34, 28, 3);
            display.drawBox(x, 37, 3, 3);
            display.drawBox(x + 13, 37, 3, 3);
            display.drawBox(x + 25, 37, 3, 3);
            x++;
        }
        if (flagx == 0 && x == 127)
        {
            flagx = 1;
        }
        // Force redraw for animation
        setDirty();
    }
}

void DashboardScreen::handleInput(InputEvent event) {
    bool success = true;

    switch(event) {
        case InputEvent::UP_PRESS: // '8' - Move Right (Legacy Logic mapped UP to Right?)
            // Legacy: key '8' -> CMD_MOVE_RIGHT_MAN
            if (!_isManualMoving) {
                if (DataManager::getInstance().sendCommand(SP::CMD_MOVE_RIGHT_MAN)) {
                    _manualCommand = ">>";
                    _isManualMoving = true;
                } else {
                    success = false;
                }
            }
            break;

        case InputEvent::DOWN_PRESS: // '0' - Move Left
            // Legacy: key '0' -> CMD_MOVE_LEFT_MAN
            if (!_isManualMoving) {
                if (DataManager::getInstance().sendCommand(SP::CMD_MOVE_LEFT_MAN)) {
                    _manualCommand = "<<";
                    _isManualMoving = true;
                } else {
                    success = false;
                }
            }
            break;

        case InputEvent::STOP_PRESS: // '6'
        case InputEvent::MANUAL_MODE_PRESS:
             // Stop manual or generic stop
             if (_isManualMoving) {
                 if (DataManager::getInstance().sendCommand(SP::CMD_STOP_MANUAL)) {
                     _manualCommand = " ";
                     _isManualMoving = false;
                 } else {
                     success = false;
                 }
             } else {
                 if (event == InputEvent::MANUAL_MODE_PRESS) {
                     DataManager::getInstance().sendCommand(SP::CMD_MANUAL_MODE);
                 } else {
                     DataManager::getInstance().sendCommand(SP::CMD_STOP);
                 }
             }
             break;

        case InputEvent::LIFT_UP_PRESS: // 'E'
            DataManager::getInstance().sendCommand(SP::CMD_LIFT_UP);
            break;

        case InputEvent::LIFT_DOWN_PRESS: // '9'
            DataManager::getInstance().sendCommand(SP::CMD_LIFT_DOWN);
            break;

        case InputEvent::BACK_PRESS: // '7'
            // Navigate to Main Menu
            ScreenManager::getInstance().push(&mainMenuScreen);
            break;

        case InputEvent::LOAD_PRESS: // '5'
             DataManager::getInstance().sendCommand(SP::CMD_LOAD);
             break;

        case InputEvent::LONG_LOAD_PRESS:
             DataManager::getInstance().sendCommand(SP::CMD_LONG_LOAD);
             break;

        case InputEvent::UNLOAD_PRESS: // 'C'
             DataManager::getInstance().sendCommand(SP::CMD_UNLOAD);
             break;

        case InputEvent::LONG_UNLOAD_PRESS:
             DataManager::getInstance().sendCommand(SP::CMD_LONG_UNLOAD);
             break;

        // Shuttle Selection Logic
        case InputEvent::KEY_A_PRESS:
        case InputEvent::KEY_B_PRESS:
             // Logic for A/B buttons to cycle shuttles?
             // Legacy: 'A' -> +1, 'B' -> -1
             {
                 int8_t current = DataManager::getInstance().getShuttleNumber();
                 if (event == InputEvent::KEY_A_PRESS) current++;
                 else current--;

                 if (current < 1) current = 1;
                 // Assuming max 32 from legacy
                 if (current > 32) current = 32;

                 DataManager::getInstance().setShuttleNumber(current);
                 DataManager::getInstance().sendCommand(SP::CMD_PING); // Check alive
             }
             break;

        default:
            // Check for digit keys 1-4 for direct selection
            if (event >= InputEvent::KEY_1_PRESS && event <= InputEvent::KEY_4_PRESS) {
                int num = (int)event - (int)InputEvent::KEY_1_PRESS + 1;
                 DataManager::getInstance().setShuttleNumber(num);
                 DataManager::getInstance().sendCommand(SP::CMD_PING);
            }
            break;
    }

    if (!success) {
        _showQueueFull = true;
        _queueFullTimer = millis();
        setDirty();
    }
}

void DashboardScreen::tick() {
    DataManager::getInstance().setPollContext(DataManager::PollContext::MAIN_DASHBOARD);

    // Check flags
    if (DataManager::getInstance().consumeTelemetryDirtyFlag()) {
        setDirty();
    }

    if (_showQueueFull && millis() - _queueFullTimer > 2000) {
        _showQueueFull = false;
        setDirty();
    }

    // Manual mode persistence logic is in DataManager now, but we update our local state if needed
    // or rely on _isManualMoving which is toggled by input.
    // Legacy code had: if (DataManager::getInstance().sendCommand(...)) isManualMoving = true;

    // Also update DataManager about manual mode
    DataManager::getInstance().setManualMoveMode(_isManualMoving);
}
