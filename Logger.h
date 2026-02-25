#pragma once
#include <Arduino.h>

// --- MASTER CONFIGURATION ---
#define DEBUG_ENABLED 1           // 1 = Logging ON, 0 = All logging code vanishes
#define APP_LOG_LEVEL 4           // 1=ERROR, 2=WARN, 3=INFO, 4=DEBUG

#if DEBUG_ENABLED
    #define LOG_INIT(baud) Serial.begin(baud)

    // Formatting: [TIME_MS][LEVEL][TAG] Message
    #define LOG_FORMAT(letter, tag, format, ...) \
        Serial.printf("[%6lu][%s][%s] " format "\r\n", millis(), #letter, tag, ##__VA_ARGS__)

    #if APP_LOG_LEVEL >= 1
        #define LOG_E(tag, format, ...) LOG_FORMAT(E, tag, format, ##__VA_ARGS__)
    #else
        #define LOG_E(tag, format, ...)
    #endif

    #if APP_LOG_LEVEL >= 2
        #define LOG_W(tag, format, ...) LOG_FORMAT(W, tag, format, ##__VA_ARGS__)
    #else
        #define LOG_W(tag, format, ...)
    #endif

    #if APP_LOG_LEVEL >= 3
        #define LOG_I(tag, format, ...) LOG_FORMAT(I, tag, format, ##__VA_ARGS__)
    #else
        #define LOG_I(tag, format, ...)
    #endif

    #if APP_LOG_LEVEL >= 4
        #define LOG_D(tag, format, ...) LOG_FORMAT(D, tag, format, ##__VA_ARGS__)
    #else
        #define LOG_D(tag, format, ...)
    #endif
#else
    #define LOG_INIT(baud)
    #define LOG_E(tag, format, ...)
    #define LOG_W(tag, format, ...)
    #define LOG_I(tag, format, ...)
    #define LOG_D(tag, format, ...)
#endif
