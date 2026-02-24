#include <AsyncElegantOTA.h>
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

namespace SP {
#include "ShuttleProtocol.h"
}

// --- Async Tx/Rx Definitions ---
enum class TxState { IDLE, WAITING_ACK, TIMEOUT_ERROR };

struct TxJob {
  uint8_t txBuffer[64];
  uint8_t txLength;
  uint8_t seqNum;
  uint32_t lastTxTime;
  uint8_t retryCount;
};

#define TX_QUEUE_SIZE 5
TxJob txQueue[TX_QUEUE_SIZE];
uint8_t txHead = 0;
uint8_t txTail = 0;
TxState txState = TxState::IDLE;

SP::ProtocolParser parser;
Preferences prefs;
SP::TelemetryPacket cachedTelemetry;
int32_t cachedConfig[16] = {0};
SP::SensorPacket cachedSensors;
SP::StatsPacket cachedStats = {0};
bool isDisplayDirty = true;
uint32_t lastDisplayUpdate = 0;
bool isOtaUpdating = false;
uint8_t nextSeqNum = 0;
bool showQueueFull = false;
uint32_t queueFullTimer = 0;

bool isManualMoving = false;
uint32_t manualHeartbeatTimer = 0;

uint32_t lastPollTime = 0;
uint32_t currentPollInterval = 400;
uint32_t lastSensorPollTime = 0;

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
#define GETDATACODE 94
int pin_code = 1441;
uint8_t pass_menu = 1;
uint8_t pass_menu_trig = 0;
int temp_pin_code = 0;
int8_t temp_pin[4] = {
  0,
};

enum Page
{
  MAIN = 1,
  MENU = 2,
  ERRORS = 3,
  UNLOAD_PALLETE = 5,
  UNLOAD_PALLETE_SUCC = 6,
  UNLOAD_PALLETE_FAIL = 7,
  OPTIONS = 8,
  CHANGE_SHUTTLE_NUM = 9,
  ERRORS_LOG = 10,
  ADDITIONAL_FUNCTIONS = 11,
  COUNT_START = 12,
  PACKING_CONTROL = 13,
  WARNINGS_LOG = 14,
  DIAGNOSTICS = 15,
  STATS = 16,
  BATTERY_PROTECTION = 17,
  ENGINEERING_MENU = 18,
  DEBUG_INFO = 19,
  SYSTEM_SETTINGS_WARN = 20,
  STATUS_PGG = 22,
  PACKING_WARN = 23,
  CHANGE_CHANNEL = 24,
  MENU_PROTECTION = 30,
  ACCESS_DENIED = 31,
  CALIBRATION = 32,
  MOVEMENT = 33,
  MOVEMENT_RIGHT = 34,
  MOVEMENT_LEFT = 35,
  UPDATE_FIRMWARE = 36,
} page;

enum Command
{
  CMD_STOP = 1,
  CMD_STOP_MANUAL = 2,
  CMD_LOAD = 3,
  CMD_UNLOAD = 4,
  CMD_CONT_LOAD = 5,
  CMD_CONT_UNLOAD = 6,
  CMD_DEMO = 7,
  CMD_PLATFORM_LIFTING = 8,
  CMD_PLATFORM_UNLIFTING = 9,
  CMD_MOVEMENT_LEFT = 10,
  CMD_MOVEMENT_RIGHT = 11,
  CMD_REVERSE_ON = 12,
  CMD_REVERSE_OFF = 13,
  CMD_INTER_PALL_DISTANCE = 14,
  CMD_UNLOAD_PALLET_BY_NUMBER = 15,
  CMD_PING = 18,
  CMD_BACK_TO_ORIGIN = 19,
  CMD_SET_SPEED = 20,
  CMD_GET_PARAM = 21,
  CMD_EVAC = 22,
  CMD_BATTERY_PROTECTION = 23,
  CMD_GET_ERRORS = 24,
  CMD_GET_MENU = 25,
  CMD_PALLETE_COUNT = 26,
  CMD_PACKING_BACK = 27,
  CMD_PACKING_FORWARD = 28,
  CMD_WAIT_TIME = 29,
  CMD_MPR_OFFSET = 30,
  CMD_CHNL_OFFSET = 31,
  CMD_FIFO_LIFO = 41,
  CMD_RESET = 45,
  CMD_MANUAL = 47,
  CMD_SET_LENGTH = 48
} command;

int8_t cursorPos = 1;  // default cursor position
int8_t shuttleNumber = 1;
int8_t shuttleTempNum = 1;

bool isUpdateStarted = false;

uint8_t inputQuant = 0;
int8_t numquant1 = 0;
int8_t numquant2 = 0;
bool displayActive = true;
bool manualMode = false;
String manualCommand = " ";
String shuttnumst = "A1";
String shuttnewnumst = "A1";
bool dispactivate = 1;
// uint8_t settDataMenu[3] = {
//   0,
// };
boolean SensorsDataTrig = false;
boolean UpdateParam = false;

const int rfout0 = 15;  // D3
const int Battery_Pin = 36;
const int Charge_Pin = 39;

const int LedRF_Pin = 21;
const int LedWiFi_Pin = 22;
// const int rfout = D4;
String Serial2in = "";

// uint8_t warncode; // Removed
int16_t speedset = 10;
int16_t newspeedset = 0;
uint8_t shuttbackaway = 100;  //отступ от края 800 паллета FIFO/LIFO
String palletconfst = "0";
bool showarn = false;
bool warnblink = false;
int8_t chargercount = 0;
bool hideshuttnum = false;
bool blinkshuttnum = false;
uint8_t pallet_plank = 0;
bool pallet_800_only = 0;
String calibret = "Нет";
int dist = 0;
char currentKey;

// byte Buffer[10];

const String shuttleStatusArray[19] = { "Запрос статуса", "Ручной режим",      "Загрузка",      "Выгрузка",
                                        "Уплотнение",     "Эвакуация",         "DEMO",          "Подсчет паллет",
                                        "Испытания",      "Обнаружены ошибки", "Ожидание...",   "Прод. загрузка",
                                        "Прод. выгрузка", "Прод. выгрузка",    "  Вперед...",   "  Назад...",
                                        "  Вверх...",     "  Вниз...",         "  Инициация..." };

const String ErrorMsgArray[21] = { "Нет ошибок",
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
                                   "",
                                   "",
                                   "" };
const String WarnMsgArray[6] = { "Нет предупреждений", "Шаттл не в канале", "Заряд < 20%",
                                 "Поддон не найден",   "Поддон поврежден",  "Нет места" };
const String shuttnum[32] = { "A1", "B2", "C3", "D4", "E5", "F6", "G7", "H8", "I9", "10", "11", "12", "13", "14", "15", "16",
                              "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30", "31", "32" };
const uint8_t shuttnumLength = sizeof(shuttnum) / sizeof(shuttnum[0]);

// uint8_t shuttleStatus = 0; // Removed
uint8_t battindicat = 0;
int chargact = 0;
int warntimer = millis();
int uctimer = warntimer;

boolean buttonActive = false;
boolean longPressActive = false;
unsigned long buttonTimer = 0;
// unsigned long displayOffTimer = 0; // Removed
// unsigned long displayOffInterval = 25000; // Removed
unsigned long longPressTime = 1000;
unsigned long mpingtime = 0;
unsigned long checkA0time = 0;
unsigned long shuttnumOffInterval = 0;
unsigned long blinkshuttnumInterval = 0;

unsigned long Elapsed;  // To calc the execution time

uint8_t errn = 1;
uint8_t warrn = 0;
uint8_t prevpercent = 100;
String strmenu[20];
Page pageAfterPin = MAIN;
#pragma endregion

#pragma region Объявление функций...
void cmdSend(uint8_t numcmd);
int getVoltage();
void BatteryLevel(uint8_t percent);
void MenuOut();
void processIncomingAck(uint8_t seq, SP::AckPacket* ack);
void handleRx();
void processTxQueue();
bool queueCommand(SP::CommandPacket packet, uint8_t targetID);
bool queueConfigSet(uint8_t paramID, int32_t value, uint8_t targetID);
bool queueRequest(uint8_t msgID, uint8_t targetID);
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
  page = MAIN;
  pinMode(rfout0, OUTPUT);
  pinMode(Charge_Pin, INPUT);
  currentKey = ' ';
  longPressActive = false;
  Serial.begin(19200);
  Serial2.begin(9600);
  // WiFi.mode(WIFI_MODE_NULL);
  // WiFi.mode(WIFI_OFF);
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

  // Initialize new Managers
  InputManager::init();
  PowerController::init();

  initRadio();

  AsyncElegantOTA.onStarted([]() { isOtaUpdating = true; });
  AsyncElegantOTA.onEnd([]() { isOtaUpdating = false; });
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
    AsyncElegantOTA.loop();
  }

  handleRx();
  processTxQueue();

  // Power Management Tick
  PowerController::tick();

  // Input Processing
  InputManager::update();
  InputEvent evt = InputManager::getNextEvent();
  if (evt != InputEvent::NONE) {
      Serial.print("Event: ");
      Serial.println((int)evt);
      PowerController::feedWatchdog();
  }

  // --- Context-Aware Polling Engine ---
  if (!isManualMoving && !isOtaUpdating)
  {
    // 1. Determine Polling Interval based on Active Page
    if (page == MAIN) {
        currentPollInterval = 400;      // Fast updates for main dashboard
    } else {
        currentPollInterval = 1500;     // Background keep-alive for other menus
    }

    // 2. Execute Heartbeat Polling
    if (millis() - lastPollTime >= currentPollInterval) {
        if (queueRequest(SP::MSG_REQ_HEARTBEAT, shuttleNumber)) {
            lastPollTime = millis();
        }
    }

    // 3. Execute Sensor Polling (Specific to DEBUG_INFO)
    if (page == DEBUG_INFO) {
        if (millis() - lastSensorPollTime >= 300) {
             // Only queue if space is available to prevent choking the heartbeat
             if (queueRequest(SP::MSG_REQ_SENSORS, shuttleNumber)) {
                lastSensorPollTime = millis();
             }
        }
    }

    if (page == STATS) {
        static uint32_t lastStatsPollTime = 0;
        if (millis() - lastStatsPollTime >= 2000) {
             if (queueRequest(SP::MSG_REQ_STATS, shuttleNumber)) {
                lastStatsPollTime = millis();
             }
        }
    }
  }

  // int Result, Status;

  String linkMode = " ";
  shuttnumst = shuttnum[shuttleNumber - 1];

  // NOTE: Display sleep logic removed, handled by PowerController now

  // char key = kpd.getKey(); // Removed
  longPressActive = false; // Reset/unused

  if (buttonActive)
  {
    // displayOffTimer = millis(); // Removed
    if ((millis() - buttonTimer > longPressTime) && (longPressActive == false)) longPressActive = true;
    if (isManualMoving)
    {
      if (millis() - manualHeartbeatTimer >= 200)
      {
        if ((txTail + 1) % TX_QUEUE_SIZE != txHead)
        {
          queueRequest(SP::MSG_REQ_HEARTBEAT, shuttleNumber);
        }
        manualHeartbeatTimer = millis();
      }
    }
  }

  // Legacy quant logic removed

  if (displayActive)
  {
    if (millis() - checkA0time > 500)
    {
      u8g2.setPowerSave(0);
      displayActive = true;
      warnblink = !warnblink;
      checkA0time = millis();
      isDisplayDirty = true;

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
    }
    if (millis() - blinkshuttnumInterval > 250)
    {
      if (hideshuttnum)
      {
        blinkshuttnum = !blinkshuttnum;
        isDisplayDirty = true;
        blinkshuttnumInterval = millis();
      }
    }
    if (millis() - shuttnumOffInterval > 5000)
    {
      isDisplayDirty = true;
      hideshuttnum = false;
      if (hideshuttnum) blinkshuttnum = 0;
    }
    if (millis() - lastDisplayUpdate >= 100)
    {
      if (isDisplayDirty || showQueueFull)
      {
        u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x13_t_cyrillic);
      u8g2.setDrawColor(1);
      // displayOffInterval = 60000;
      if (page == MAIN)
      {
        // displayOffInterval = 25000;
        u8g2.setCursor(0, 10);
        u8g2.print("Заряд"); 
        u8g2.setCursor(35, 10);
        if (cachedTelemetry.batteryCharge == 101)
          u8g2.print("[errb] ");
        else if (cachedTelemetry.shuttleStatus == 0)
          u8g2.print("[err] ");
        else
          u8g2.print("[" + String(cachedTelemetry.batteryCharge) + "%] ");
        u8g2.setCursor(75, 10);
        if (cachedTelemetry.stateFlags & 0x20)
          u8g2.print("LIFO");
        else
          u8g2.print("FIFO");
        u8g2.setCursor(0, 25);
        if (cachedTelemetry.shuttleStatus == 13)
          u8g2.print("Осталось выгрузить " + String(cachedTelemetry.palleteCount));
        else
          u8g2.print(shuttleStatusArray[cachedTelemetry.shuttleStatus]);

        if (showQueueFull) {
            u8g2.setCursor(0, 40);
            u8g2.print("! QUEUE FULL !");
            if (millis() - queueFullTimer > 2000) {
                showQueueFull = false;
                isDisplayDirty = true;
            }
        } else if (cachedTelemetry.errorCode)
        {
          u8g2.setCursor(0, 40);
          if (cachedTelemetry.errorCode < 21)
             u8g2.print("! " + ErrorMsgArray[cachedTelemetry.errorCode] + " !");
          else
             u8g2.print("! ERR " + String(cachedTelemetry.errorCode) + " !");
        }
        // Legacy warncode logic removed/replaced by errorCode display

        // u8g2.setCursor(100, 30);
        // u8g2.print(String(analogRead(A0)));

        if (chargact)
          BatteryLevel(battindicat);
        else
          BatteryLevel(getVoltage());
        u8g2.setFont(u8g2_font_10x20_t_cyrillic);
        if (!hideshuttnum)
        {
          u8g2.setCursor(30 * (shuttleNumber - 1), 63);
          if (shuttleNumber > 4)
          {
            u8g2.setCursor(100, 63);
          }
          u8g2.print("|" + String(shuttleNumber) + "|");
        }
        else if (blinkshuttnum)
        {
          u8g2.setCursor(30 * (shuttleTempNum - 1), 63);
          if (shuttleTempNum > 4)
          {
            u8g2.setCursor(100, 63);
          }
          u8g2.print("|" + String(shuttleTempNum) + "|");
        }
        if (showarn && warnblink)
        {
          u8g2.setCursor(120, 30);
          u8g2.print("!");
        }
        if (cachedTelemetry.shuttleStatus == 13)
        {
          uint8_t j = 0;
          for (uint8_t i = 0; i < cachedTelemetry.palleteCount; i++)
          {
            if (j > 125) break;
            u8g2.drawBox(j, 32, 3, 12);
            j += 6;
          }
        }
        u8g2.setFont(u8g2_font_6x13_t_cyrillic);
        if (manualMode)
        {
          u8g2.setCursor(85, 40);
          u8g2.print(manualCommand);
        }
        if (buttonActive)
        {
          u8g2.setCursor(0, 40);
          u8g2.print("_");
          // u8g2.print(String(esp_sleep_get_wakeup_cause()));

          // u8g2.setCursor(40, 40);
          // u8g2.print(currentKey);
        }
        if (Serial2in != "")
        {
          u8g2.setCursor(0, 40);
          u8g2.print(".");
        }

        if (cachedTelemetry.shuttleStatus == 3 || cachedTelemetry.shuttleStatus == 13 || cachedTelemetry.shuttleStatus == 4 || cachedTelemetry.shuttleStatus == 6)
        {
          static uint8_t x = 0;
          static uint8_t flagx = 0;
          u8g2.drawBox(x, 40, 28, 5);
          u8g2.drawBox(x + 4, 45, 3, 2);
          u8g2.drawBox(x + 21, 45, 3, 2);
          if (flagx == 0 && x < 128)
            x++;
          else
          {
            flagx = 1;
            u8g2.drawBox(x, 34, 28, 3);
            u8g2.drawBox(x, 37, 3, 3);
            u8g2.drawBox(x + 13, 37, 3, 3);
            u8g2.drawBox(x + 25, 37, 3, 3);
            x--;
          }
          if (flagx == 1 && x == 0)
          {
            flagx = 0;
          }
        }
        else if (cachedTelemetry.shuttleStatus == 2 || cachedTelemetry.shuttleStatus == 12)
        {
          static uint8_t x = 128;
          static uint8_t flagx = 1;
          u8g2.drawBox(x, 40, 28, 5);
          u8g2.drawBox(x + 4, 45, 3, 2);
          u8g2.drawBox(x + 21, 45, 3, 2);

          if (flagx == 1 && x > 1)
            x--;
          else
          {
            flagx = 0;
            u8g2.drawBox(x, 34, 28, 3);
            u8g2.drawBox(x, 37, 3, 3);
            u8g2.drawBox(x + 13, 37, 3, 3);
            u8g2.drawBox(x + 25, 37, 3, 3);
            x++;
          }
          if (flagx == 0 && x == 127)
          {
            flagx = 1;
          }
        }
      }
      else if (page == MENU)
      {
        String shuttbacka = "";
        String fifolifo_modest = "";
        if (shuttbackaway)
          shuttbacka = "вкл ";
        else
          shuttbacka = "выкл";
        if (cachedTelemetry.stateFlags & 0x20)
          fifolifo_modest = "LIFO ";
        else
          fifolifo_modest = "FIFO";
        strmenu[0] = " Подсчет палет: " + palletconfst;
        strmenu[1] = " Уплотнение";
        strmenu[2] = " Режим: " + fifolifo_modest;
        strmenu[3] = " Настройки";
        strmenu[4] = " Ошибки";
        strmenu[5] = " Выгрузка N палет";
        strmenu[6] = " Вернуться в нач. поз.";
        strmenu[7] = " Назад";
        MenuOut();
      }
      else if (page == ERRORS)
      {
        errn = 0;
        warrn = 0;
        for (uint8_t i = 0; i < 16; i++)
        {
          if (bitRead(cachedTelemetry.errorCode, i))
          {
            errn++;
          }
        }
        // Legacy warncode logic removed
        if (errn)
          strmenu[0] = " Ошибки: " + String(errn - 1);
        else
          strmenu[0] = " Ошибки: 0";
        strmenu[1] = " Сброс ошибок";
        strmenu[2] = " Назад";
        strmenu[3] = "  ";
        strmenu[4] = "  ";
        MenuOut();
      }
      else if (page == UNLOAD_PALLETE)
      {
        inputQuant = numquant1 * 10 + numquant2;
        Serial.println("Quant 1 = " + String(inputQuant));
        u8g2.setCursor(0, 5 + 1 * 11);
        u8g2.print(" Выгрузить N палет:");
        u8g2.setFont(u8g2_font_9x15_t_cyrillic);
        u8g2.setCursor(0, 12 + 2 * 11);
        if (numquant1 == 0)
          u8g2.print(" 0" + String(inputQuant));
        else
          u8g2.print(" " + String(inputQuant));
        uint8_t sline;
        if (cursorPos == 3)
          sline = 24;
        else
          sline = 8;
        u8g2.drawBox(9 * cursorPos, 37, sline, 3);
      }
      else if (page == UNLOAD_PALLETE_SUCC)
      {
        u8g2.setCursor(0, 5 + 1 * 11);
        u8g2.print(" Выгрузка " + String(inputQuant) + " палет");
        u8g2.setCursor(0, 5 + 2 * 11);
        u8g2.print(" запущена");
      }
      else if (page == UNLOAD_PALLETE_FAIL)
      {
        u8g2.setCursor(0, 5 + 1 * 11);
        u8g2.print(" Вы ввели " + String(inputQuant));
        u8g2.setCursor(0, 5 + 2 * 11);
        u8g2.print(" отмена");
      }
      else if (page == OPTIONS)
      {
        String txtMode = " ";
        if (!fifolifo)
        {
          txtMode = ">>";
        }
        else
        {
          txtMode = "<<";
        }
        strmenu[0] = " МПР: " + String(mpr) + " mm";
        strmenu[1] = " Инверсия движения:" + txtMode;
        strmenu[2] = " Макс. скорость: " + String(speedset / 10);
        strmenu[3] = " Изменить N уст-ва";
        strmenu[4] = " Защита батареи: " + String(lowbatt) + "%";
        strmenu[5] = " Инж. меню";
        strmenu[6] = " Сохр. параметров";
        strmenu[7] = " Назад";
        MenuOut();
      }
      else if (page == CHANGE_SHUTTLE_NUM)
      {
        u8g2.setCursor(0, 5 + 1 * 11);
        u8g2.print(" Введите");
        u8g2.setCursor(0, 5 + 2 * 11);
        u8g2.print(" новый номер уст-ва");
        u8g2.setCursor(0, 5 + 3 * 11);
        u8g2.print(" " + String(shuttleTempNum));
      }
      else if (page == CHANGE_CHANNEL)
      {
        u8g2.setCursor(0, 5 + 1 * 11);
        u8g2.print(" Введите");
        u8g2.setCursor(0, 5 + 2 * 11);
        u8g2.print(" новый номер канала");
        u8g2.setCursor(0, 5 + 3 * 11);
        u8g2.print(" " + String(tempChannelNumber));
      }
      else if (page == ERRORS_LOG)
      {
        uint8_t ec;
        uint8_t epos = 1;
        for (ec = 1; ec < 16; ec++)
        {
          if (bitRead(cachedTelemetry.errorCode, ec))
          {
            u8g2.setCursor(0, 5 + epos * 11);
            u8g2.print(String(" E#0" + String(ec) + " " + ErrorMsgArray[ec]));
            epos++;
          }
        }
        if (epos == 1)
        {
          u8g2.setCursor(0, 16);
          u8g2.print("Нет ошибок");
        }
      }
      else if (page == COUNT_START)
      {
        u8g2.setCursor(0, 5 + 1 * 11);
        u8g2.print(" Подсчет");
        u8g2.setCursor(0, 5 + 2 * 11);
        u8g2.print(" запущен");
      }
      else if (page == PACKING_CONTROL)
      {
        String tempcomp = " Уплотнение";
        strmenu[0] = tempcomp + " вперед";
        strmenu[1] = tempcomp + " назад";
        strmenu[2] = " Назад";
        strmenu[3] = " ";
        strmenu[4] = " ";
        MenuOut();
      }
      else if (page == WARNINGS_LOG)
      {
        // u8g2.print(String(WarnMsgArray[warncode]));
        u8g2.print("Logs moved to console");
      }
      else if (page == BATTERY_PROTECTION)
      {
        strmenu[0] = " Мин. заряд: [N/A]";
        strmenu[1] = " Назад";
        strmenu[2] = " ";
        strmenu[3] = " ";
        strmenu[4] = " ";
        MenuOut();
      }
      else if (page == ENGINEERING_MENU)
      {
        strmenu[0] = " Калибровка";
        strmenu[1] = " Отладка";
        strmenu[2] = " Систем. настр.";
        strmenu[3] = " Журналирование";
        strmenu[4] = " Движение";
        strmenu[5] = " Изменить канал";
        strmenu[6] = " Длинна шаттла: " + String(shuttleLength);
        strmenu[7] = " Время ож.загр: " + String(waittime);
        strmenu[8] = " Смещение МПР: " + String(mproffset);
        strmenu[9] = " Смещение канала: " + String(chnloffset);
        strmenu[10] = " Статистика";
        strmenu[11] = " Назад";
        MenuOut();
      }
      else if (page == DEBUG_INFO && cursorPos == 1)
      {
        uint16_t sym_code = 0;
        u8g2.setFont(u8g2_font_6x13_t_cyrillic);
        u8g2.setCursor(2, 22);
        u8g2.print("Forw. chnl dist: " + String(cachedSensors.distanceF));
        u8g2.setCursor(2, 32);
        u8g2.print("Rvrs. chnl dist: " + String(cachedSensors.distanceR));
        u8g2.setCursor(2, 42);
        u8g2.print("Forw. plt dist:  " + String(cachedSensors.distancePltF));
        u8g2.setCursor(2, 52);
        u8g2.print("Rvrs. plt dist:  " + String(cachedSensors.distancePltR));
        u8g2.setCursor(2, 62);
        u8g2.print("Encoder ang: " + String(cachedSensors.angle));

        u8g2.setFont(u8g2_font_unifont_t_symbols);
        static uint8_t count_timer = 0;
        if (SensorsDataTrig) count_timer++;
        if (count_timer > 3)
        {
          count_timer = 0;
        }
        sym_code = 0x25f7 - count_timer;
        u8g2.drawGlyph(120, 10, sym_code);
      }
      else if (page == DEBUG_INFO && cursorPos == 2)
      {
        u8g2.setFont(u8g2_font_6x13_t_cyrillic);
        for(int i=0; i<8; i++) {
            bool state = (cachedSensors.hardwareFlags & (1 << i)) != 0;
            u8g2.setCursor(2, 10 + i*10);
            if (i == 0) u8g2.print(String(state) + " DATCHIK_F1");
            else u8g2.print(String(state) + " SENSOR_" + String(i));
        }
      }
      else if (page == STATS)
      {
        u8g2.setFont(u8g2_font_6x13_t_cyrillic);
        u8g2.setCursor(0, 10);
        u8g2.print("Дистанция: " + String(cachedStats.totalDist / 1000) + "м");
        u8g2.setCursor(0, 22);
        u8g2.print("Циклов загр: " + String(cachedStats.loadCounter));
        u8g2.setCursor(0, 34);
        u8g2.print("Аварий: " + String(cachedStats.crashCount));
        u8g2.setCursor(0, 46);
        u8g2.print("Uptime: " + String(cachedStats.totalUptimeMinutes) + "мин");

        u8g2.setCursor(0, 60);
        u8g2.print("7 - Назад");
      }

      else if (page == SYSTEM_SETTINGS_WARN)
      {
        u8g2.setCursor(40, 15);
        String passw = " ";
        if (cursorPos != 4)
          passw = "!";
        else
          passw = "*";
        u8g2.print("ВНИМАНИЕ" + passw);
        u8g2.setCursor(0, 26);
        u8g2.print("  Изменение   данных");
        u8g2.setCursor(0, 37);
        u8g2.print(" пар-в может привести");
        u8g2.setCursor(0, 48);
        u8g2.print("к повреждению устр-ва");
        u8g2.setCursor(0, 63);
        u8g2.print("< назад     вперед ОК");
      }
      else if (page == 21)
      {
        if (!UpdateParam)
        {
          strmenu[0] = " Считать пар-ры ";
        }
        else
        {
          strmenu[0] = " Пар-ры считаны";
        }
        strmenu[1] = " Номер шатт " + String(cachedConfig[SP::CFG_SHUTTLE_NUM]);
        strmenu[2] = " Номер экр " + String(cachedConfig[SP::CFG_CHNL_OFFSET]);
        strmenu[3] = " away_set -"; // + String(bitRead(settDataMenu[2], 0));
        strmenu[4] = " enc_invers " + String(cachedConfig[SP::CFG_REVERSE_MODE]);
        strmenu[5] = " slim -"; // + String(bitRead(settDataMenu[2], 2));
        strmenu[6] = " d_canal_off -"; // + String(bitRead(settDataMenu[2], 3));
        strmenu[7] = " away_filifo -"; // + String(bitRead(settDataMenu[2], 4));
        strmenu[8] = " FIFO_MODE " + String(cachedConfig[SP::CFG_FIFO_LIFO]);
        strmenu[9] = " demo_en -"; // + String(bitRead(settDataMenu[2], 6));
        strmenu[10] = " crane_stat -"; // + String(bitRead(settDataMenu[2], 7));
        strmenu[11] = " stop_aft_pall " + String(cachedConfig[SP::CFG_INTER_PALLET]);
        strmenu[12] = " upl_wait " + String(cachedConfig[SP::CFG_WAIT_TIME]);
        strmenu[13] = " pall_plk " + String(pallet_plank);
        strmenu[14] = " pall_800 " + String(pallet_800_only);
        strmenu[15] = R_VERSION;
        strmenu[16] = " ";
        strmenu[17] = " Записать пар-ры";
        strmenu[18] = " Назад";
        MenuOut();
      }
      else if (page == STATUS_PGG)
      {
        strmenu[0] = " Параметры журналировпания";
        if (logWrite)
          strmenu[1] = " Журналирование: ВКЛ";
        else
          strmenu[1] = " Журналирование: ВЫКЛ";
        strmenu[2] = " Назад";
        strmenu[3] = "";
        strmenu[4] = "";
        strmenu[5] = "";
        MenuOut();
      }
      else if (page == PACKING_WARN)
      {
        u8g2.setCursor(0, 15);
        u8g2.print(" Перед использованием");
        u8g2.setCursor(0, 26);
        u8g2.print(" убедитесь в наличии");
        u8g2.setCursor(0, 37);
        u8g2.print(" свободного паллето-");
        u8g2.setCursor(0, 48);
        u8g2.print(" места в канале!");
        u8g2.setCursor(0, 63);
        u8g2.print("< назад     вперед ОК");
      }
      else if (page == MENU_PROTECTION)
      {
        temp_pin_code = temp_pin[0] * 1000 + temp_pin[1] * 100 + temp_pin[2] * 10 + temp_pin[3];
        u8g2.setCursor(0, 5 + 1 * 11);
        u8g2.print(" PIN CODE:");
        u8g2.setFont(u8g2_font_9x15_t_cyrillic);
        u8g2.setCursor(0, 12 + 2 * 11);
        String temp_z = "";
        if (temp_pin_code < 10)
        {
          u8g2.print(" 000" + String(temp_pin_code));
        }
        else if (temp_pin_code >= 10 && temp_pin_code < 100)
        {
          u8g2.print(" 00" + String(temp_pin_code));
        }
        else if (temp_pin_code >= 100 && temp_pin_code < 1000)
        {
          u8g2.print(" 000" + String(temp_pin_code));
        }
        else
          u8g2.print(" " + String(temp_pin_code));
        u8g2.drawBox(9 * cursorPos, 37, 8, 3);
      }
      else if (page == ACCESS_DENIED)
      {
        u8g2.setCursor(0, 5 + 1 * 11);
        u8g2.print(" Доступ");
        u8g2.setCursor(0, 5 + 2 * 11);
        u8g2.print(" запрещен");
      }
      else if (page == CALIBRATION)
      {
        strmenu[0] = " Выполнено:" + String(calibret);
        strmenu[1] = " Сохранить";
        strmenu[2] = " Назад";
        strmenu[3] = "";
        strmenu[4] = "";
        strmenu[5] = "";
        MenuOut();
      }
      else if (page == MOVEMENT)
      {
        strmenu[0] = " Движение >>";
        strmenu[1] = " Движение <<";
        strmenu[2] = " Назад";
        strmenu[3] = "";
        strmenu[4] = "";
        strmenu[5] = "";
        MenuOut();
      }
      else if (page == MOVEMENT_RIGHT)
      {
        strmenu[0] = " 10см";
        strmenu[1] = " 20см";
        strmenu[2] = " 30см";
        strmenu[3] = " 50см";
        strmenu[4] = " 100см";
        strmenu[5] = " 200см";
        strmenu[6] = " 300см";
        strmenu[7] = " 500см";
        strmenu[8] = "Назад";
        MenuOut();
      }
      else if (page == MOVEMENT_LEFT)
      {
        strmenu[0] = " 10см";
        strmenu[1] = " 20см";
        strmenu[2] = " 30см";
        strmenu[3] = " 50см";
        strmenu[4] = " 100см";
        strmenu[5] = " 200см";
        strmenu[6] = " 300см";
        strmenu[7] = " 500см";
        strmenu[8] = "Назад";
        MenuOut();
      }

        u8g2.sendBuffer();
        isDisplayDirty = false;
      }
      lastDisplayUpdate = millis();

      if (page == UNLOAD_PALLETE_SUCC || page == UNLOAD_PALLETE_FAIL || page == ACCESS_DENIED)
      {
        delay(1200);
        page = MAIN;
        isDisplayDirty = true;
      }
    }
  }
  else
    digitalWrite(rfout0, HIGH);
}

#pragma region Функции...
void cmdSend(uint8_t numcmd)
{
  SP::CommandPacket cmd;
  cmd.arg1 = 0;
  cmd.arg2 = 0;
  uint8_t target = shuttleNumber;

  switch (numcmd)
  {
    case CMD_STOP:
      cmd.cmdType = SP::CMD_STOP;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      manualMode = false;
      manualCommand = " ";
      break;
    case CMD_STOP_MANUAL:
      cmd.cmdType = SP::CMD_STOP_MANUAL;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      manualMode = false;
      manualCommand = " ";
      break;
    case CMD_LOAD:
      cmd.cmdType = SP::CMD_LOAD;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      manualMode = false;
      break;
    case CMD_UNLOAD:
      cmd.cmdType = SP::CMD_UNLOAD;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      manualMode = false;
      break;
    case CMD_CONT_LOAD:
      cmd.cmdType = SP::CMD_LONG_LOAD;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      manualMode = false;
      break;
    case CMD_CONT_UNLOAD:
      cmd.cmdType = SP::CMD_LONG_UNLOAD;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      manualMode = false;
      break;
    case CMD_DEMO:
      cmd.cmdType = SP::CMD_DEMO;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      manualMode = false;
      break;
    case CMD_PLATFORM_LIFTING:
      cmd.cmdType = SP::CMD_LIFT_UP;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      manualCommand = "/\\";
      break;
    case CMD_PLATFORM_UNLIFTING:
      cmd.cmdType = SP::CMD_LIFT_DOWN;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      manualCommand = "\\/";
      break;
    case CMD_MOVEMENT_LEFT:
      cmd.cmdType = SP::CMD_MOVE_LEFT_MAN;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      manualCommand = "<<";
      break;
    case CMD_MOVEMENT_RIGHT:
      cmd.cmdType = SP::CMD_MOVE_RIGHT_MAN;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      manualCommand = ">>";
      break;
    case CMD_UNLOAD_PALLET_BY_NUMBER:
      cmd.cmdType = SP::CMD_LONG_UNLOAD_QTY;
      cmd.arg1 = inputQuant;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      break;
    case 16:
      if (!queueRequest(SP::MSG_REQ_HEARTBEAT, target)) { showQueueFull = true; queueFullTimer = millis(); }
      break;
    case 17:
      if (!queueConfigSet(SP::CFG_SHUTTLE_NUM, shuttleTempNum, target)) { showQueueFull = true; queueFullTimer = millis(); }
      break;
    case CMD_PING:
      cmd.cmdType = SP::CMD_PING;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      break;
    case CMD_BACK_TO_ORIGIN:
      cmd.cmdType = SP::CMD_HOME;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      break;
    case CMD_GET_PARAM:
      if (!queueRequest(SP::MSG_REQ_STATS, target)) { showQueueFull = true; queueFullTimer = millis(); }
      break;
    case CMD_GET_ERRORS:
      if (!queueRequest(SP::MSG_REQ_HEARTBEAT, target)) { showQueueFull = true; queueFullTimer = millis(); }
      break;
    case CMD_GET_MENU:
      if (!queueRequest(SP::MSG_REQ_HEARTBEAT, target)) { showQueueFull = true; queueFullTimer = millis(); }
      break;
    case CMD_PALLETE_COUNT:
      cmd.cmdType = SP::CMD_COUNT_PALLETS;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      break;
    case CMD_PACKING_BACK:
      cmd.cmdType = SP::CMD_COMPACT_R;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      break;
    case CMD_PACKING_FORWARD:
      cmd.cmdType = SP::CMD_COMPACT_F;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      break;
    case 43:
      cmd.cmdType = SP::CMD_CALIBRATE;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      break;
    case 44:
      if (!queueRequest(SP::MSG_REQ_SENSORS, target)) { showQueueFull = true; queueFullTimer = millis(); }
      break;
    case CMD_RESET:
      cmd.cmdType = SP::CMD_RESET_ERROR;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      break;
    case CMD_MANUAL:
      cmd.cmdType = SP::CMD_MANUAL_MODE;
      if (!queueCommand(cmd, target)) { showQueueFull = true; queueFullTimer = millis(); }
      break;
  }
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
  // volt = volt / 100;
  // Serial2.print("A0 "); Serial2.println(raw);
  // Serial2.print("Voltage "); Serial2.println(volt);
  return volt;
}

void BatteryLevel(uint8_t percent)
{
  uint8_t xsize = 0;
  if (percent < prevpercent || percent > prevpercent + 5)
  {
    prevpercent = percent;
  }
  if (prevpercent > 95)
    xsize = 14;
  else if (prevpercent > 75)
    xsize = 11;
  else if (prevpercent > 50)
    xsize = 8;
  else if (prevpercent > 25)
    xsize = 5;
  else if (prevpercent > 7)
    xsize = 2;
  else
    xsize = 0;
  u8g2.drawFrame(107, 1, 18, 10);
  u8g2.drawBox(125, 4, 2, 4);
  if (xsize) u8g2.drawBox(109, 3, xsize, 6);
  // u8g2.setCursor(100, 25);
  // u8g2.print(String(percent));
  // u8g2.print(String(analogRead(Battery_Pin)));
  // if (digitalRead(Charge_Pin)) u8g2.print(" ON");
  // else u8g2.print(" OFF");
}
void MenuOut()
{
  if (cursorPos < 6)
    u8g2.drawBox(0, 6 + (cursorPos - 1) * 11, 128, 11);
  else
    u8g2.drawBox(0, 50, 128, 11);
  for (uint8_t i = 1; i < 6; i++)
  {
    u8g2.setCursor(0, 5 + i * 11);
    if (cursorPos == i || (cursorPos > 5 && i == 5))
      u8g2.setDrawColor(0);
    else
      u8g2.setDrawColor(1);
    if (cursorPos < 6)
      u8g2.print(strmenu[i - 1]);
    else
      u8g2.print(strmenu[cursorPos + i - 6]);
  }
}

bool queueCommand(SP::CommandPacket cmd, uint8_t targetID) {
  uint8_t nextTail = (txTail + 1) % TX_QUEUE_SIZE;
  if (nextTail != txHead) {
    TxJob* job = &txQueue[txTail];
    job->seqNum = nextSeqNum++;
    job->retryCount = 0;

    SP::FrameHeader* header = (SP::FrameHeader*)job->txBuffer;
    header->sync1 = SP::PROTOCOL_SYNC_1;
    header->sync2 = SP::PROTOCOL_SYNC_2;
    header->length = sizeof(SP::CommandPacket);
    header->targetID = targetID;
    header->seq = job->seqNum;
    header->msgID = SP::MSG_COMMAND;

    memcpy(job->txBuffer + sizeof(SP::FrameHeader), &cmd, sizeof(SP::CommandPacket));
    SP::ProtocolUtils::appendCRC(job->txBuffer, sizeof(SP::FrameHeader) + sizeof(SP::CommandPacket));
    job->txLength = sizeof(SP::FrameHeader) + sizeof(SP::CommandPacket) + 2;

    txTail = nextTail;
    return true;
  }
  return false;
}

bool queueRequest(uint8_t msgID, uint8_t targetID) {
  uint8_t nextTail = (txTail + 1) % TX_QUEUE_SIZE;
  if (nextTail != txHead) {
    TxJob* job = &txQueue[txTail];
    job->seqNum = nextSeqNum++;
    job->retryCount = 0;

    SP::FrameHeader* header = (SP::FrameHeader*)job->txBuffer;
    header->sync1 = SP::PROTOCOL_SYNC_1;
    header->sync2 = SP::PROTOCOL_SYNC_2;
    header->length = 0;
    header->targetID = targetID;
    header->seq = job->seqNum;
    header->msgID = msgID;

    SP::ProtocolUtils::appendCRC(job->txBuffer, sizeof(SP::FrameHeader));
    job->txLength = sizeof(SP::FrameHeader) + 2;

    txTail = nextTail;
    return true;
  }
  return false;
}

bool queueConfigSet(uint8_t paramID, int32_t value, uint8_t targetID) {
  uint8_t nextTail = (txTail + 1) % TX_QUEUE_SIZE;
  if (nextTail != txHead) {
    TxJob* job = &txQueue[txTail];
    job->seqNum = nextSeqNum++;
    job->retryCount = 0;

    SP::ConfigPacket cfg;
    cfg.paramID = paramID;
    cfg.value = value;

    SP::FrameHeader* header = (SP::FrameHeader*)job->txBuffer;
    header->sync1 = SP::PROTOCOL_SYNC_1;
    header->sync2 = SP::PROTOCOL_SYNC_2;
    header->length = sizeof(SP::ConfigPacket);
    header->targetID = targetID;
    header->seq = job->seqNum;
    header->msgID = SP::MSG_CONFIG_SET;

    memcpy(job->txBuffer + sizeof(SP::FrameHeader), &cfg, sizeof(SP::ConfigPacket));
    SP::ProtocolUtils::appendCRC(job->txBuffer, sizeof(SP::FrameHeader) + sizeof(SP::ConfigPacket));
    job->txLength = sizeof(SP::FrameHeader) + sizeof(SP::ConfigPacket) + 2;

    txTail = nextTail;
    return true;
  }
  return false;
}

void processIncomingAck(uint8_t seq, SP::AckPacket* ack) {
  if (txState == TxState::WAITING_ACK && txHead != txTail) {
    if (txQueue[txHead].seqNum == seq) {
      txHead = (txHead + 1) % TX_QUEUE_SIZE;
      txState = TxState::IDLE;
    }
  }
}

void processTxQueue() {
  if (isOtaUpdating) return;

  if (txState == TxState::IDLE) {
    if (txHead != txTail) {
      TxJob* job = &txQueue[txHead];

      if (Serial2.availableForWrite() >= job->txLength) {
          Serial2.write(job->txBuffer, job->txLength);
          job->lastTxTime = millis();
          txState = TxState::WAITING_ACK;
      }
    }
  } else if (txState == TxState::WAITING_ACK) {
    if (txHead != txTail) {
      TxJob* job = &txQueue[txHead];
      if (millis() - job->lastTxTime > 500) {
        if (job->retryCount < 3) {
          if (Serial2.availableForWrite() >= job->txLength) {
              job->retryCount++;
              Serial2.write(job->txBuffer, job->txLength);
              job->lastTxTime = millis();
          }
        } else {
          txState = TxState::TIMEOUT_ERROR;
        }
      }
    } else {
      txState = TxState::IDLE;
    }
  } else if (txState == TxState::TIMEOUT_ERROR) {
      if (txHead != txTail) {
        txHead = (txHead + 1) % TX_QUEUE_SIZE;
      }
      txState = TxState::IDLE;
  }
}

void handleRx() {
  if (isOtaUpdating) return;

  while (Serial2.available()) {
    uint8_t byte = Serial2.read();
    SP::FrameHeader* header = parser.feed(byte);

    if (header) {
      uint8_t* payload = (uint8_t*)header + sizeof(SP::FrameHeader);

      switch (header->msgID) {
        case SP::MSG_ACK:
          processIncomingAck(header->seq, (SP::AckPacket*)payload);
          break;

        case SP::MSG_HEARTBEAT:
          {
            if (header->length == sizeof(SP::TelemetryPacket)) {
                if (memcmp(&cachedTelemetry, payload, sizeof(SP::TelemetryPacket)) != 0) {
                    memcpy(&cachedTelemetry, payload, sizeof(SP::TelemetryPacket));
                    isDisplayDirty = true;
                }
            }
          }
          break;

        case SP::MSG_SENSORS:
          {
            if (header->length == sizeof(SP::SensorPacket)) {
                if (memcmp(&cachedSensors, payload, sizeof(SP::SensorPacket)) != 0) {
                    memcpy(&cachedSensors, payload, sizeof(SP::SensorPacket));
                    isDisplayDirty = true;
                }
            }
          }
          break;

        case SP::MSG_CONFIG_REP:
          {
            if (header->length == sizeof(SP::ConfigPacket)) {
                SP::ConfigPacket* pkt = (SP::ConfigPacket*)payload;
                if (pkt->paramID < 16) {
                    cachedConfig[pkt->paramID] = pkt->value;
                    UpdateParam = true;
                    isDisplayDirty = true;
                }
            }
          }
          break;

        case SP::MSG_STATS:
          {
            if (header->length == sizeof(SP::StatsPacket)) {
                if (memcmp(&cachedStats, payload, sizeof(SP::StatsPacket)) != 0) {
                    memcpy(&cachedStats, payload, sizeof(SP::StatsPacket));
                    isDisplayDirty = true;
                }
            }
          }
          break;
      }
    }
  }
}
#pragma endregion