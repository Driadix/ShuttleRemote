#include "PowerController.h"
#include "DataManager.h"
#include <driver/rtc_io.h>
#include "Logger.h"

unsigned long PowerController::_lastActivityTime = 0;
unsigned long PowerController::_sleepThreshold = 25000; // 25s default
bool PowerController::_preventSleep = false;

int PowerController::_batteryPin = -1;
int PowerController::_chargePin = -1;
unsigned long PowerController::_lastBatteryCheck = 0;
int PowerController::_chargerCount = 0;
int PowerController::_battIndicator = 0;

#define PIN_RFOUT0 15
#define BUTTON_PIN_BITMASK 0x308005000

void PowerController::init(int batteryPin, int chargePin) {
    _batteryPin = batteryPin;
    _chargePin = chargePin;
    pinMode(_chargePin, INPUT);

    _lastActivityTime = millis();
    
    // Release holds from previous sleep state
    rtc_gpio_init((gpio_num_t)13);
    rtc_gpio_set_direction((gpio_num_t)13, RTC_GPIO_MODE_OUTPUT_ONLY);
    gpio_hold_dis((gpio_num_t)13);
    
    rtc_gpio_init((gpio_num_t)25);
    rtc_gpio_set_direction((gpio_num_t)25, RTC_GPIO_MODE_OUTPUT_ONLY);
    gpio_hold_dis((gpio_num_t)25);
    
    rtc_gpio_init((gpio_num_t)26);
    rtc_gpio_set_direction((gpio_num_t)26, RTC_GPIO_MODE_OUTPUT_ONLY);
    gpio_hold_dis((gpio_num_t)26);

    // Inputs
    rtc_gpio_init((gpio_num_t)12);
    rtc_gpio_set_direction((gpio_num_t)12, RTC_GPIO_MODE_INPUT_ONLY);
    gpio_set_pull_mode((gpio_num_t)12, GPIO_PULLDOWN_ONLY);
    gpio_hold_dis((gpio_num_t)12);
    
    rtc_gpio_init((gpio_num_t)14);
    rtc_gpio_set_direction((gpio_num_t)14, RTC_GPIO_MODE_INPUT_ONLY);
    gpio_set_pull_mode((gpio_num_t)14, GPIO_PULLDOWN_ONLY);
    gpio_hold_dis((gpio_num_t)14);
    
    rtc_gpio_init((gpio_num_t)27);
    rtc_gpio_set_direction((gpio_num_t)27, RTC_GPIO_MODE_INPUT_ONLY);
    gpio_set_pull_mode((gpio_num_t)27, GPIO_PULLDOWN_ONLY);
    gpio_hold_dis((gpio_num_t)27);
    
    rtc_gpio_init((gpio_num_t)32);
    rtc_gpio_set_direction((gpio_num_t)32, RTC_GPIO_MODE_INPUT_ONLY);
    gpio_set_pull_mode((gpio_num_t)32, GPIO_PULLDOWN_ONLY);
    gpio_hold_dis((gpio_num_t)32);
    
    rtc_gpio_init((gpio_num_t)33);
    rtc_gpio_set_direction((gpio_num_t)33, RTC_GPIO_MODE_INPUT_ONLY);
    gpio_set_pull_mode((gpio_num_t)33, GPIO_PULLDOWN_ONLY);
    gpio_hold_dis((gpio_num_t)33);

    // Enable wakeup
    esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
}

void PowerController::tick() {
    // Battery Monitor
    if (_batteryPin != -1 && millis() - _lastBatteryCheck > 500) {
        _lastBatteryCheck = millis();

        bool isCharging = false;
        if (!digitalRead(_chargePin)) {
            _chargerCount = 8;
        } else {
            _chargerCount--;
            if (_chargerCount <= 0) _chargerCount = 0;
        }

        if (_chargerCount > 0) {
            isCharging = true;
        }

        static bool lastChargingState = false;
        if (isCharging != lastChargingState) {
             LOG_D("PWR", "Charging state changed: %s", isCharging ? "CHARGING" : "DISCHARGING");
             lastChargingState = isCharging;
        }

        int raw = analogRead(_batteryPin);
        int volt = map(raw, 740, 1100, 0, 100);
        if (volt < 0) volt = 0;
        else if (volt > 100) volt = 100;

        DataManager::getInstance().setRemoteBatteryLevel(volt, isCharging);
    }

    if (_preventSleep) {
        _lastActivityTime = millis();
        return;
    }
    
    if (millis() - _lastActivityTime > _sleepThreshold) {
        enterDeepSleep();
    }
}

void PowerController::feedWatchdog() {
    _lastActivityTime = millis();
}

void PowerController::preventSleep(bool prevent) {
    _preventSleep = prevent;
    if (prevent) {
        feedWatchdog();
    }
}

void PowerController::enterDeepSleep() {
    LOG_I("PWR", "Initiating deep sleep. Holding power pins...");
    // Configure pins for sleep state
    rtc_gpio_init((gpio_num_t)13);
    rtc_gpio_set_direction((gpio_num_t)13, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level((gpio_num_t)13, 1);
    gpio_hold_en((gpio_num_t)13);
    
    rtc_gpio_init((gpio_num_t)25);
    rtc_gpio_set_direction((gpio_num_t)25, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level((gpio_num_t)25, 1);
    gpio_hold_en((gpio_num_t)25);
    
    rtc_gpio_init((gpio_num_t)26);
    rtc_gpio_set_direction((gpio_num_t)26, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level((gpio_num_t)26, 1);
    gpio_hold_en((gpio_num_t)26);

    rtc_gpio_init((gpio_num_t)12);
    rtc_gpio_set_direction((gpio_num_t)12, RTC_GPIO_MODE_INPUT_ONLY);
    gpio_set_pull_mode((gpio_num_t)12, GPIO_PULLDOWN_ONLY);
    gpio_hold_en((gpio_num_t)12);
    
    rtc_gpio_init((gpio_num_t)14);
    rtc_gpio_set_direction((gpio_num_t)14, RTC_GPIO_MODE_INPUT_ONLY);
    gpio_set_pull_mode((gpio_num_t)14, GPIO_PULLDOWN_ONLY);
    gpio_hold_en((gpio_num_t)14);
    
    rtc_gpio_init((gpio_num_t)27);
    rtc_gpio_set_direction((gpio_num_t)27, RTC_GPIO_MODE_INPUT_ONLY);
    gpio_set_pull_mode((gpio_num_t)27, GPIO_PULLDOWN_ONLY);
    gpio_hold_en((gpio_num_t)27);
    
    rtc_gpio_init((gpio_num_t)32);
    rtc_gpio_set_direction((gpio_num_t)32, RTC_GPIO_MODE_INPUT_ONLY);
    gpio_set_pull_mode((gpio_num_t)32, GPIO_PULLDOWN_ONLY);
    gpio_hold_en((gpio_num_t)32);
    
    rtc_gpio_init((gpio_num_t)33);
    rtc_gpio_set_direction((gpio_num_t)33, RTC_GPIO_MODE_INPUT_ONLY);
    gpio_set_pull_mode((gpio_num_t)33, GPIO_PULLDOWN_ONLY);
    gpio_hold_en((gpio_num_t)33);
    
    // rfout0 handling
    pinMode(PIN_RFOUT0, OUTPUT);
    digitalWrite(PIN_RFOUT0, LOW);
    gpio_hold_en((gpio_num_t)PIN_RFOUT0);

    delay(100);
    esp_deep_sleep_start();
}
