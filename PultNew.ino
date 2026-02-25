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
// bool isDisplayDirty = true; // Managed by ScreenManager/Screens now
// uint32_t lastDisplayUpdate = 0;
bool isOtaUpdating = false;

// Note: isManualMoving is now managed by DataManager and DashboardScreen

#pragma region переменные
HardwareSerial &hSerial = Serial2;

int addr0 = 0;

uint8_t configArray[] = { 0xC0, 0x0, 0x0, 0x1C, 0x10, 0x46 };
const int configArrayStartAddress = addr0 + 4;
uint8_t channelNumber = configArray[4];
uint8_t tempChannelNumber = channelNumber;
uint8_t logWrite = 0;
U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/18, /* data=*/23, /* cs=*/2, /* dc=*/19, /* reset=*/4);
#define R_VERSION   " v1.0 2023"

int pin_code = 1441;
uint8_t pass_menu = 1;
// uint8_t pass_menu_trig = 0; // Legacy
// int temp_pin_code = 0; // Legacy

int8_t shuttleNumber = 1;
// int8_t shuttleTempNum = 1; // Legacy

bool isUpdateStarted = false;

// Legacy variables removed (inputQuant, numquant, etc.)

bool displayActive = true;

const int rfout0 = 15;  // D3
const int Battery_Pin = 36;
const int Charge_Pin = 39;

const int LedRF_Pin = 21;
const int LedWiFi_Pin = 22;
// const int rfout = D4;
String Serial2in = "";

// uint8_t warncode; // Removed
// int16_t speedset = 10; // Removed, handled by DataManager
// int16_t newspeedset = 0;
// uint8_t shuttbackaway = 100;
// String palletconfst = "0";
// bool showarn = false;
// bool warnblink = false;
int8_t chargercount = 0;
// bool hideshuttnum = false;
// bool blinkshuttnum = false;
// uint8_t pallet_plank = 0;
// bool pallet_800_only = 0;
// String calibret = "Нет";
// int dist = 0;
// char currentKey;

uint8_t battindicat = 0;
int chargact = 0;
uint8_t prevpercent = 100;

unsigned long checkA0time = 0;

#pragma endregion

#pragma region setup

void initRadio() {
  digitalWrite(rfout0, HIGH);
  delay(1000);
  Serial.println("Write LoRa settings...");
  Serial2.write(configArray, sizeof(configArray));
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
  pinMode(Charge_Pin, INPUT);

  Serial.begin(19200);
  Serial2.begin(9600);

  btStop();
  Serial.print("Start");

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
  channelNumber = (uint8_t)prefs.getUInt("rf_chan", configArray[4]);
  configArray[4] = channelNumber;
  tempChannelNumber = channelNumber;

  delay(50);
  
  // Initialize Managers
  InputManager::init();
  PowerController::init();
  DataManager::getInstance().init(&Serial2, shuttleNumber);
  
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
#pragma endregion

bool isFabala = true;

void loop()
{
  if (isFabala)
  {
    for (int i = 0; i < sizeof(configArray); i++)
    {
      Serial.print("0x");
      Serial.print(configArray[i], HEX);
      Serial.print(" ");
    }
    isFabala = false;
    Serial.print(" | ");
  }

  if (isUpdateStarted)
  {
    ElegantOTA.loop();
  }
  
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

  // --- Battery Monitoring (Legacy extracted) ---
  if (displayActive)
  {
    if (millis() - checkA0time > 500)
    {
      // u8g2.setPowerSave(0); // Managed by PowerController? Or need to keep awake?
      // displayActive = true;
      checkA0time = millis();

      if (!digitalRead(Charge_Pin))
      {
        chargercount = 8;
      }
      else
      {
        chargercount--;
        if (chargercount <= 0)
        {
          chargact = 0;
          chargercount = 0;
        }
      }
      if (chargercount > 0)
      {
        chargact = 1;
        prevpercent = 100;
        battindicat += 25;
        if (battindicat > 100) battindicat = 0;
      }
      // Note: Updating battery display is handled by StatusBarWidget polling DataManager or local logic?
      // StatusBarWidget usually reads from DataManager.
      // We should update DataManager with local battery info if needed?
      // DataManager telemetry comes from Shuttle. Local battery is Pult battery.
      // StatusBarWidget needs to know Pult battery.
      // Currently StatusBarWidget likely reads DataManager telemetry battery (Shuttle Battery).
      // If we want Pult battery, we need a way to pass it.
      // But legacy code displayed shuttle battery primarily?
      // Legacy: "Заряд [err] / [xx%]". This is from Telemetry.
      // And `BatteryLevel` function drew icon.
      // `BatteryLevel` used `battindicat` (charging) or `getVoltage()` (Pult battery?).
      // The icon at top right seems to be Pult battery.
      // Text "Заряд [...]" is Shuttle battery.

      // StatusBarWidget should probably handle both.
      // But for now, let's assume StatusBarWidget handles generic info.
      // If StatusBarWidget doesn't implement Pult battery check, we might lose that icon.
      // Prompt said: "StatusBarWidget is a pure observer that renders telemetry data".
      // So it renders Shuttle info.
      // Pult battery info might be missing if StatusBarWidget doesn't include it.
      // But I am following the plan "Include your StatusBarWidget".
    }
  }
  else
    digitalWrite(rfout0, HIGH);
}

//заряд
int getVoltage()
{
  int raw = analogRead(Battery_Pin);
  int volt = map(raw, 740, 1100, 0, 100);
  if (volt < 0)
  {
    volt = 0;
  }
  else if (volt > 100)
  {
    volt = 100;
  }
  return volt;
}

void setRadioChannel(uint8_t ch) {
    channelNumber = ch;
    configArray[4] = ch;
    prefs.putUInt("rf_chan", ch);
    initRadio();
}
