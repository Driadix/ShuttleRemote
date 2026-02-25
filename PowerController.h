#ifndef POWER_CONTROLLER_H
#define POWER_CONTROLLER_H

#include <Arduino.h>

class PowerController {
public:
    static void init(int batteryPin, int chargePin);
    static void tick();
    static void feedWatchdog();
    static void preventSleep(bool prevent);
    
private:
    static void enterDeepSleep();
    static unsigned long _lastActivityTime;
    static unsigned long _sleepThreshold;
    static bool _preventSleep;

    // Battery Monitor
    static int _batteryPin;
    static int _chargePin;
    static unsigned long _lastBatteryCheck;
    static int _chargerCount;
    static int _battIndicator; // Legacy animator state
};

#endif // POWER_CONTROLLER_H
