#include <ElegantOTA.h>
#include <AsyncTCP.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <driver/rtc_io.h>
#include "InputEvents.h"
#include "InputManager.h"
#include "PowerController.h"
#include "DataManager.h"
#include "ScreenManager.h"
#include "UI_Graph.h"

// --- Global UI State ---
Preferences prefs;
bool isOtaUpdating = false;

// Note: isManualMoving is now managed by DataManager and DashboardScreen

HardwareSerial &hSerial = Serial2;

// Config Array Replaced by Local Generation in initRadio()
U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 2, /* dc=*/ 19, /* reset=*/ 4);
#define R_VERSION   " v1.0 2023"

int pin_code = 1441;
uint8_t pass_menu = 1;

int8_t shuttleNumber = 1;

bool displayActive = true;

const int rfout0 = 15;  // D3

void initRadio() {
  digitalWrite(rfout0, HIGH);
  delay(1000);
  Serial.println("Write LoRa settings...");

  uint8_t config[6] = { 0xC0, 0x0, 0x0, 0x1C, 0x00, 0x46 };
  config[4] = DataManager::getInstance().getRadioChannel();

  Serial2.write(config, sizeof(config));
  delay(100);
  digitalWrite(rfout0, LOW);

  // Flush buffer to remove any config response or garbage
  Serial2.flush();
  while (Serial2.available()) {
    Serial2.read();
  }
}

void setup()
{
  pinMode(rfout0, OUTPUT);
  // Charge Pin mode set in PowerController

  Serial.begin(19200);
  Serial2.begin(9600);

  btStop();
  Serial.print("Start");

  SPI.begin(18, 35, 23, 2);
  delay(100);

  u8g2.begin();
  u8g2.enableUTF8Print();
#ifndef mk
  u8g2.setFlipMode(1);
#endif

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_10x20_t_cyrillic);
  u8g2.setCursor(10, 30);
  u8g2.print("М И К Р О Н");
  u8g2.setFont(u8g2_font_4x6_t_cyrillic);
  u8g2.drawFrame(3, 7, 125, 33);
  u8g2.sendBuffer();
  delay(100);

  displayActive = true;

  prefs.begin("pult_cfg", false);
  shuttleNumber = (int8_t)prefs.getUInt("sht_num", 1);
  pass_menu = (uint8_t)prefs.getUInt("pass_menu", 1);
  uint8_t savedChannel = (uint8_t)prefs.getUInt("rf_chan", 0x10);

  delay(50);
  
  // Initialize Managers
  InputManager::init();
  const int BATTERY_PIN = 36;
  const int CHARGE_PIN = 39;
  PowerController::init(BATTERY_PIN, CHARGE_PIN);
  DataManager::getInstance().init(&Serial2, shuttleNumber);
  DataManager::getInstance().setRadioChannel(savedChannel);
  
  initRadio();

  // Initialize Screen Manager
  ScreenManager::getInstance().push(&dashboardScreen);

  ElegantOTA.onStart([]() {
      isOtaUpdating = true;
      DataManager::getInstance().setOtaUpdating(true);
  });
  ElegantOTA.onEnd([](bool success) {
      isOtaUpdating = false;
      DataManager::getInstance().setOtaUpdating(false);
  });
}

void loop()
{
  // --- DataManager Tick ---
  DataManager::getInstance().tick();

  // Polling context is now managed by Screens via Screen::tick() calling DataManager::setPollContext()
  // But we need to ensure ScreenManager::tick() calls active screen's tick().

  // --- Dirty Flag Consumption ---
  // Screens handle consuming dirty flags in their tick() methods.
  
  // Power Management Tick
  PowerController::tick();
  
  // Input Processing
  InputManager::update();
  InputEvent evt = InputManager::getNextEvent();
  if (evt != InputEvent::NONE) {
      Serial.print("Event: ");
      Serial.println((int)evt);
      PowerController::feedWatchdog();
      ScreenManager::getInstance().handleInput(evt);
  }

  // --- Display Update ---
  if (!isOtaUpdating) {
      ScreenManager::getInstance().tick(u8g2);
  }

}

void setRadioChannel(uint8_t ch) {
    DataManager::getInstance().setRadioChannel(ch);
    prefs.putUInt("rf_chan", ch);
    initRadio();
}
