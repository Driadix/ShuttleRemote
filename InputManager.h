#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>
#include "InputEvents.h"

class InputManager {
public:
    static void init();
    static void update();
    static InputEvent getNextEvent();

private:
    // Internal state tracking
    static InputEvent _queuedEvent;
};

#endif // INPUT_MANAGER_H
