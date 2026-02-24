#include "InputManager.h"
#include <Keypad.h>

InputEvent InputManager::_queuedEvent = InputEvent::NONE;

// Keypad Configuration
const byte ROWS = 5;
const byte COLS = 3;

// Map from existing code
char keys[ROWS][COLS] = {
    { '1', '2', 'A' },
    { '3', '4', 'B' },
    { '5', '6', 'C' },
    { '7', '8', 'D' },
    { '9', '0', 'E' }
};

byte rowPins[ROWS] = { 32, 33, 27, 14, 12 };
byte colPins[COLS] = { 25, 26, 13 };

Keypad kpd = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Internal state
static unsigned long buttonTimer = 0;
static bool longPressActive = false;
static const unsigned long longPressThreshold = 1000;
static char activeKey = 0;

void InputManager::init() {
    // Keypad initialized by constructor
    // Set hold time to match our threshold if we were using listener, but we are polling manually
    // kpd.setHoldTime(longPressThreshold);
}

static InputEvent mapKeyToEvent(char key, bool isLong) {
    if (isLong) {
        switch (key) {
            case 'D': return InputEvent::OK_LONG_PRESS;
            case '6': return InputEvent::MANUAL_MODE_PRESS;
            case '5': return InputEvent::LONG_LOAD_PRESS;
            case 'C': return InputEvent::LONG_UNLOAD_PRESS;
            default: return InputEvent::NONE;
        }
    } else {
        switch (key) {
            case '8': return InputEvent::UP_PRESS;
            case '0': return InputEvent::DOWN_PRESS;
            case 'D': return InputEvent::OK_SHORT_PRESS;
            case '7': return InputEvent::BACK_PRESS;
            case '6': return InputEvent::STOP_PRESS;
            case '5': return InputEvent::LOAD_PRESS;
            case 'C': return InputEvent::UNLOAD_PRESS;
            case '9': return InputEvent::LIFT_DOWN_PRESS;
            case 'E': return InputEvent::LIFT_UP_PRESS;
            case '1': return InputEvent::KEY_1_PRESS;
            case '2': return InputEvent::KEY_2_PRESS;
            case '3': return InputEvent::KEY_3_PRESS;
            case '4': return InputEvent::KEY_4_PRESS;
            case 'A': return InputEvent::KEY_A_PRESS;
            case 'B': return InputEvent::KEY_B_PRESS;
            default: return InputEvent::NONE;
        }
    }
}

void InputManager::update() {
    // Scan keys
    if (kpd.getKeys()) {
        for (int i = 0; i < LIST_MAX; i++) {
            if (kpd.key[i].stateChanged) {
                char key = kpd.key[i].kchar;
                switch (kpd.key[i].kstate) {
                    case PRESSED:
                        activeKey = key;
                        buttonTimer = millis();
                        longPressActive = false;
                        break;

                    case RELEASED:
                        if (key == activeKey) {
                            if (!longPressActive) {
                                // It was a short press
                                _queuedEvent = mapKeyToEvent(key, false);
                            }
                            // Reset state
                            activeKey = 0;
                            longPressActive = false;
                        }
                        break;

                    default:
                        break;
                }
            }
        }
    }

    // Check for long press timeout
    if (activeKey != 0 && !longPressActive) {
        if (millis() - buttonTimer > longPressThreshold) {
            longPressActive = true;
            InputEvent evt = mapKeyToEvent(activeKey, true);
            if (evt != InputEvent::NONE) {
                _queuedEvent = evt;
            }
        }
    }
}

InputEvent InputManager::getNextEvent() {
    InputEvent evt = _queuedEvent;
    _queuedEvent = InputEvent::NONE;
    return evt;
}
