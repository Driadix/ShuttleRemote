#ifndef POWER_CONTROLLER_H
#define POWER_CONTROLLER_H

#include <Arduino.h>

class PowerController {
public:
    static void init();
    static void tick();
    static void feedWatchdog();
    static void preventSleep(bool prevent);

private:
    static void enterDeepSleep();
    static unsigned long _lastActivityTime;
    static unsigned long _sleepThreshold;
    static bool _preventSleep;
};

#endif // POWER_CONTROLLER_H
