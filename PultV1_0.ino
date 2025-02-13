#include <EEPROM.h>
#include <SPI.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <Keypad.h>
#include <U8g2lib.h>
#include <driver/rtc_io.h>
//#include <esp_deep_sleep.h>
#include "sleep_m.h"

#pragma region переменные
HardwareSerial &hSerial = Serial2;

int addr0 = 0;

uint8_t configArray[] = { 0xC0, 0x0, 0x0, 0x1C, 0x10, 0x46 };
const int configArrayStartAddress = addr0 + 4;
uint8_t channelNumber = configArray[4];
uint8_t tempChannelNumber = channelNumber;
uint8_t logWrite = 0;
bool isConfigArrayChanged = false;
U8G2_SSD1306_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/18, /* data=*/23, /* cs=*/2, /* dc=*/19, /* reset=*/4);
#define R_VERSION " v1.0 2023"
#define GETDATACODE 94
byte I2CADDR = 0x38;
int pin_code = 1441;
uint8_t pass_menu = 1;
uint8_t pass_menu_trig = 0;
int temp_pin_code = 0;
int8_t temp_pin[4] = {
  0,
};

const char *ssid = "ESP32UPDATE";
const char *password = "1234567890";
const int channel = 10;        // WiFi Channel number between 1 and 13
const bool hide_SSID = false;  // To disable SSID broadcast -> SSID will not appear in a basic WiFi scan
const int max_connection = 2;


AsyncWebServer server(80);

#define BUTTON_PIN_BITMASK 0x308005000  // GPIOs 12, 14, 27, 33, 32

#ifdef mk
#else
const byte ROWS = 5;  //строки
const byte COLS = 3;  //столбцы
// карта клавиатуры
char keys[ROWS][COLS] = {
  { '1', '2', 'A' },
  { '3', '4', 'B' },
  { '5', '6', 'C' },
  { '7', '8', 'D' },
  { '9', '0', 'E' }
};

enum Page {
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

enum Command {
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

// к каким выводам подключено
byte rowPins[ROWS] = { 32, 33, 27, 14, 12 };
byte colPins[COLS] = { 25, 26, 13 };

#endif
//объект клавиатуры
Keypad kpd(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

int8_t cursorPos = 1;  // default cursor position
int8_t shuttleNumber = 1;
int8_t shuttleTempNum = 1;
uint16_t shuttleLength = 800;
uint16_t newshuttleLength = 0;

bool isUpdateStarted = false;

int mpr = 0;
int newMpr = 0;
bool calibr = false;
uint8_t quant = 0;
int8_t numquant1 = 0;
int8_t numquant2 = 0;
bool fifolifo = false;
bool rfLink = true;
bool displayActive = true;
bool manualMode = false;
String manualCommand = " ";
String shuttnumst = "A1";
String shuttnewnumst = "A1";
bool dispactivate = 1;
bool displayUpdate = true;
uint8_t settDataIn[40];
uint16_t SensorsData[14];
uint8_t settDataMenu[3] = {
  0,
};
uint8_t settStatus[5] = {
  0,
};
uint8_t DataSend[20] = {
  0,
};
boolean SensorsDataTrig = false;
boolean UpdateParam = false;
uint32_t dist_stop_cv3 = 0;
uint32_t upl_wait_time = 0;
uint32_t enc_mm = 0;
uint16_t sensor_channel_f = 0;
uint16_t sensor_channel_r = 0;
uint16_t sensor_channel_df = 0;
uint16_t sensor_channel_dr = 0;
uint16_t sensor_pallete_F = 0;
uint16_t sensor_pallete_R = 0;
uint16_t sensor_pallete_DF = 0;
uint16_t sensor_pallete_DR = 0;
uint8_t DATCHIK_F1 = 0;
uint8_t DATCHIK_F2 = 0;
uint8_t DATCHIK_R1 = 0;
uint8_t DATCHIK_R2 = 0;

uint16_t sens_mm = 0;
uint16_t stop_mm = 0;

uint8_t ShuttGetsData[6];

const int rfout0 = 15;  //D3
const int Battery_Pin = 36;
const int Charge_Pin = 39;

const int LedRF_Pin = 21;
const int LedWiFi_Pin = 22;
//const int rfout = D4;
String Serial2in = "";

long interval = 1000;
long RSSI = 0;
//int bars = 0;
int bares = 0;
uint8_t invMode;
uint8_t speedCurr;
int8_t shuttleBattery = -1;
uint16_t errorcode;
uint8_t warncode;
String tempcode;
uint8_t countcharge = 0;  //count repeat request charge and status
int16_t speedset = 10;
int16_t newspeedset = 0;
uint8_t palletconf = 0;       // get quant pallets
uint8_t shuttbackaway = 100;  //отступ от края 800 паллета FIFO/LIFO
uint8_t fifolifo_mode = 0;    //режим fifo/lifo
String palletconfst = "0";
bool blink = 0;
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
//ESP.wdtFeed();
//byte mac[] = { 0x28, 0x63, 0x36, 0xEF, 0x47, 0x8B };

//byte Buffer[10];

String inputString = "";         // a String to hold incoming data from Serial2
boolean stringComplete = false;  // whether the string is complete

String connectionStatus;
const String shuttleStatusArray[19] = { "Запрос статуса", "Ручной режим", "Загрузка", "Выгрузка", "Уплотнение", "Эвакуация", "DEMO", "Подсчет паллет", "Испытания", "Обнаружены ошибки", "Ожидание...", "Прод. загрузка", "Прод. выгрузка", "Прод. выгрузка", "  Вперед...", "  Назад...", "  Вверх...", "  Вниз...", "  Инициация..."};

const String ErrorMsgArray[21] = { "Нет ошибок", "Сенсор канала F", "Сенсор канала R", "Сенсор канала DF", "Сенсор канала DR", "Сенсор паллет F", "Сенсор паллет R", "Сенсор паллет DF", "Сенсор паллет DR", "Подъемник", "Привод движ.", "Низкий заряд", "Столкновение", "Перегрев", "", "", "" };
const String WarnMsgArray[6] = { "Нет предупреждений", "Шаттл не в канале", "Заряд < 20%", "Поддон не найден", "Поддон поврежден",  "Нет места"};
const String shuttnum[26] = { "A1", "B2", "C3", "D4", "E5", "F6", "G7", "H8", "I9", "J10", "K11", "L12", "M13", "N14", "O15", "P16", "Q17", "R18", "S19", "T20", "U21", "V22", "W23", "X24", "Y25", "Z26" };

const String num_no_yes[2] = { "НЕТ", "ДА" };
//const String shutt_allbuttons[8] = { "upl", "dow", "com", "mmd", "dmd", "sgh", "saw", "cof" };  //upload, download, compact, manual mode, demo mode, shutt go home, step away, compact forw
uint8_t evacuatstatus = 0;
uint8_t shuttleStatus = 0;
uint8_t lowbatt = 20;
uint8_t newlowbatt = 100;
uint8_t battindicat = 0;
uint8_t newreverse = 2;
uint8_t waittime = 0;
uint8_t newwaittime = 0;
int8_t mproffset = 0;
int8_t newmproffset = 120;
int8_t chnloffset = 0;
int8_t newchnloffset = 120;
int chargact = 0;
uint8_t testnum = 1;
int testtimer1 = 0;
int testtimer2 = 0;
int testrepeat = 1;
int warntimer = millis();
int uctimer = warntimer;

boolean buttonActive = false;
boolean longPressActive = false;
unsigned long buttonTimer = 0;
unsigned long displayOffTimer = 0;
unsigned long displayOffInterval = 25000;
unsigned long longPressTime = 1000;
unsigned long mpingtime = 0;
unsigned long getchargetime = 0;
unsigned long checkA0time = 0;
unsigned long shuttnumOffInterval = 0;
unsigned long blinkshuttnumInterval = 0;

unsigned long Elapsed;  // To calc the execution time
int LastStatus = -1;    // To force the first print
int8_t minlevel = 31;

uint8_t errn = 1;
uint8_t warrn = 0;
uint8_t prevpercent = 100;
uint8_t cyclegetcharge = 0;
String strmenu[20];
uint32_t statistic[10];  // = {0,0,0,0,0,0,0,0,0,0};
uint8_t odometr_pos = 0;
Page pageAfterPin = MAIN;
#pragma endregion

#pragma region Объявление функций...
void cmdSend(uint8_t numcmd);
int getVoltage();
void keypadEvent(KeypadEvent key);
void PinSetpcf();
void MarkTime();
void ShowTime();
void GetSerial2Data();
void SetSleep();
void BatteryLevel(uint8_t percent);
void MenuOut();
#pragma endregion

void setup() {
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
  //u8g2.setCursor(0, 60);
  //u8g2.print("@ 2019 ОНТ All Rights Reserved");
  u8g2.drawFrame(3, 7, 125, 33);
  u8g2.sendBuffer();
  delay(100);

  displayActive = true;
  EEPROM.begin(10);
  uint8_t shtnum = EEPROM.read(addr0);
  pass_menu = EEPROM.read(addr0 + 1);
  if (pass_menu != 0 && pass_menu != 1) pass_menu = 0;
  if (shtnum == 0 || shtnum > 26) {
    shtnum = 1;
    EEPROM.write(addr0, shuttleNumber);
    delay(50);
  } else shuttleNumber = shtnum;
  if (EEPROM.read(configArrayStartAddress) == 0xC0 || EEPROM.read(configArrayStartAddress) == 0xC2) {
    for (int i = 0; i < sizeof(configArray); i++) {
      configArray[i] = EEPROM.read(configArrayStartAddress + i);
    }
    if (channelNumber != configArray[4]) {
      channelNumber = configArray[4];
      tempChannelNumber = channelNumber;
    }
  }
  EEPROM.end();

  //shuttleNumber=EEPROM.read(addr0)+1;


  delay(50);
  kpd.addEventListener(keypadEvent);
  cmdSend(16);
  countcharge = 0;
  getchargetime = millis();
  displayOffInterval = 25000;
  rtc_gpio_init((gpio_num_t)13);
  rtc_gpio_set_direction((gpio_num_t)13, RTC_GPIO_MODE_OUTPUT_ONLY);
  gpio_hold_dis((gpio_num_t)13);
  rtc_gpio_init((gpio_num_t)25);
  rtc_gpio_set_direction((gpio_num_t)25, RTC_GPIO_MODE_OUTPUT_ONLY);
  gpio_hold_dis((gpio_num_t)25);
  rtc_gpio_init((gpio_num_t)26);
  rtc_gpio_set_direction((gpio_num_t)26, RTC_GPIO_MODE_OUTPUT_ONLY);
  gpio_hold_dis((gpio_num_t)26);
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
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  // Прописываем параметры связи радиомодуля
  digitalWrite(rfout0, HIGH);
  delay(1000);
  Serial.println("Write LoRa settings...");
  Serial2.write(configArray, sizeof(configArray));
  delay(100);
  Serial2.write(0xC1);
  Serial2.write(0xC1);
  Serial2.write(0xC1);
  delay(25);
  while (Serial2.available())  // Получаем параметры и выводим их в монитор порта
  {
    int8_t inByte = Serial2.read();
    Serial.print(inByte, HEX);
    Serial.print(" ");
  }
  digitalWrite(rfout0, LOW);
}

bool isFabala = true;

void loop() {
  if (isFabala) {
    for (int i = 0; i < sizeof(configArray); i++) {
      Serial.print("0x");
      Serial.print(configArray[i], HEX);
      Serial.print(" ");
    }
    isFabala = false;
    EEPROM.begin(10);
    Serial.print(" EEPROM CHANNEL: ");
    Serial.print(EEPROM.read(configArrayStartAddress + 4));
    EEPROM.end();
    Serial.print(" | ");
  }

  if (isUpdateStarted) {
    AsyncElegantOTA.loop();
  }
  GetSerial2Data();
  //int Result, Status;

  String linkMode = " ";
  shuttnumst = shuttnum[shuttleNumber - 1];
  if (millis() - displayOffTimer > displayOffInterval) {
    if (displayActive) {
      u8g2.clearBuffer();  //очистить буфер
      u8g2.drawBitmap(0, 0, 16, 64, sleep_mode);
      u8g2.sendBuffer();  //рисовать содержимое буфера
      delay(1000);
      u8g2.setPowerSave(1);
      displayActive = false;
      dispactivate = 0;
      page = MAIN;
      SetSleep();
    }
  } else {
    gpio_hold_dis((gpio_num_t)rfout0);
    digitalWrite(rfout0, HIGH);
  }
  char key = kpd.getKey();
  longPressActive = false;

  if (buttonActive) {
    displayOffTimer = millis();
    if ((millis() - buttonTimer > longPressTime) && (longPressActive == false)) longPressActive = true;
    if (manualMode && page == MAIN && (currentKey == '8' || currentKey == '0')) {
      if (millis() - mpingtime > 150) {
        mpingtime = millis();
        cmdSend(CMD_PING);
      }
    }
  }

  if (quant && millis() - uctimer > 500 && shuttleStatus != 13 && shuttleStatus != 12) {
    if (quant == 1) {cmdSend(CMD_UNLOAD); shuttleStatus = 12; quant = 0;}
    else {cmdSend(CMD_UNLOAD_PALLET_BY_NUMBER); shuttleStatus = 13;}
  }

  if (displayActive) {

    if (millis() - checkA0time > 500) {
      u8g2.setPowerSave(0);
      displayActive = true;
      warnblink = !warnblink;
      checkA0time = millis();
      displayUpdate = true;

      if (cyclegetcharge > 60) {
        if (buttonActive == 0) {
          countcharge = 0;
          getchargetime = millis();
        }
        cyclegetcharge = 0;
      } else if (shuttleStatus == 11 && buttonActive == 0 && (cyclegetcharge == 20 || cyclegetcharge == 60 || cyclegetcharge == 100)) {
        cmdSend(35);
      }
      cyclegetcharge++;
      if (!digitalRead(Charge_Pin)) {
        chargercount = 8;
      } else {
        chargercount--;
        if (chargercount <= 0) {
          chargact = 0;
          chargercount = 0;
        }
      }
      if (chargercount > 0) {
        chargact = 1;
        prevpercent = 100;
        battindicat += 25;
        if (battindicat > 100) battindicat = 0;
      }
      if (page == DEBUG_INFO && SensorsDataTrig) cmdSend(44);
    }

    if (millis() - getchargetime > 300) {
      uint8_t cc = 3;
      if (page == STATS) cc = 6;
      if (countcharge < cc) {
        if (page == MAIN) {
          if (countcharge == 0) shuttleStatus = 18;
          else if (countcharge == 2 && shuttleStatus == 18) {
            shuttleStatus = 0;
            shuttleBattery = -1;
            errorcode = 0;
            warncode = 0;
            manualMode = false;
            newMpr = 0;
            quant = 0;
            newshuttleLength = 0;
            newspeedset = 0;
            newlowbatt = 100;
            newreverse = 2;
            waittime = 0;
            newwaittime = 0;
            mproffset = 0;
            newmproffset = 120;
            chnloffset = 0;
            newchnloffset = 120;
          }
          cmdSend(16);
        } else if (page == MENU) cmdSend(CMD_GET_MENU);  //cmdGetData();
        else if (page == ERRORS) cmdSend(CMD_GET_ERRORS);
        else if (page == OPTIONS) cmdSend(CMD_GET_PARAM);
        else if (page == DIAGNOSTICS) cmdSend(34);
        else if (page == STATS) cmdSend(36);
        else if (page == BATTERY_PROTECTION) cmdSend(38);
        //else if (page==11) cmdPalletConf();
        countcharge++;
        getchargetime = millis();
      }
    }
    if (millis() - blinkshuttnumInterval > 250) {
      if (hideshuttnum) {
        blinkshuttnum = !blinkshuttnum;
        displayUpdate = true;
        blinkshuttnumInterval = millis();
      }
    }
    if (millis() - shuttnumOffInterval > 5000) {
      displayUpdate = true;
      hideshuttnum = false;
      if (hideshuttnum) blinkshuttnum = 0;
    }
    if (displayUpdate) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x13_t_cyrillic);
      u8g2.setDrawColor(1);
      //displayOffInterval = 60000;
      if (page == MAIN) {
        //displayOffInterval = 25000;
        u8g2.setCursor(0, 10);
        u8g2.print("Заряд");  //["+String(shuttleNumber)+"]");
        u8g2.setCursor(35, 10);
        if (shuttleBattery == 101) u8g2.print("[errb] ");
        else if (shuttleStatus == 0 || shuttleBattery < 0) u8g2.print("[err] ");
        else u8g2.print("[" + String(shuttleBattery) + "%] ");
        u8g2.setCursor(75, 10);
        if (fifolifo_mode) u8g2.print("LIFO");
        else u8g2.print("FIFO");
        u8g2.setCursor(0, 25);
        if (shuttleStatus == 13) u8g2.print("Осталось выгрузить " + String(quant));
        else if (quant && millis() - uctimer < 500) u8g2.print("Выгрузка " + String(quant) + " паллет");
        else u8g2.print(shuttleStatusArray[shuttleStatus]);
        if (warncode) {
          u8g2.setCursor(0, 40);
          u8g2.print("! " + WarnMsgArray[warncode] + " !");
          if (warncode == 3 || warncode == 4 || warncode == 5) {
            quant = 0;
            if (shuttleStatus != 2 && shuttleStatus != 3 && shuttleStatus != 4 && shuttleStatus != 6 && shuttleStatus != 11 && shuttleStatus != 12) {
              u8g2.setCursor(0, 25);
              u8g2.print(" Возврат         ");
            }
          }
        }
        if ((warncode == 3 || warncode == 4 || warncode == 5) && millis() - warntimer > 10000) warncode = 0;

        //u8g2.setCursor(100, 30);
        //u8g2.print(String(analogRead(A0)));

        if (chargact) BatteryLevel(battindicat);
        else BatteryLevel(getVoltage());
        u8g2.setFont(u8g2_font_10x20_t_cyrillic);
        if (!hideshuttnum) {
          u8g2.setCursor(30 * (shuttleNumber - 1), 63);
          if (shuttleNumber > 4) {
            u8g2.setCursor(100, 63);
          }
          u8g2.print("|" + String(shuttleNumber) + "|");
        } else if (blinkshuttnum) {
          u8g2.setCursor(30 * (shuttleTempNum - 1), 63);
          if (shuttleTempNum > 4) {
            u8g2.setCursor(100, 63);
          }
          u8g2.print("|" + String(shuttleTempNum) + "|");
        }
        if (showarn && warnblink) {
          u8g2.setCursor(120, 30);
          u8g2.print("!");
        }
        if (shuttleStatus == 13) {
          //u8g2.setCursor(60, 45);
          //u8g2.print(String(quant));
          uint8_t j = 0;
          for (uint8_t i = 0; i < quant; i++) {
            if (j > 125) break;
            u8g2.drawBox(j, 32, 3, 12);
            j += 6;
          }
        }
        u8g2.setFont(u8g2_font_6x13_t_cyrillic);
        if (manualMode) {
          u8g2.setCursor(85, 40);
          u8g2.print(manualCommand);
        }
        if (buttonActive) {
          u8g2.setCursor(0, 40);
          u8g2.print("_");
          //u8g2.print(String(esp_sleep_get_wakeup_cause()));

          //u8g2.setCursor(40, 40);
          //u8g2.print(currentKey);
        }
        if (Serial2in != "") {
          u8g2.setCursor(0, 40);
          u8g2.print(".");
        }

        if (shuttleStatus == 3 || shuttleStatus == 13 || shuttleStatus == 4 || shuttleStatus == 6) {  //|| shuttleStatus == 11) {
          static uint8_t x = 0;
          static uint8_t flagx = 0;
          u8g2.drawBox(x, 40, 28, 5);
          u8g2.drawBox(x + 4, 45, 3, 2);
          u8g2.drawBox(x + 21, 45, 3, 2);
          if (flagx == 0 && x < 128) x++;
          else {
            flagx = 1;
            u8g2.drawBox(x, 34, 28, 3);
            u8g2.drawBox(x, 37, 3, 3);
            u8g2.drawBox(x + 13, 37, 3, 3);
            u8g2.drawBox(x + 25, 37, 3, 3);
            x--;
          }
          if (flagx == 1 && x == 0) {
            flagx = 0;
          }
        } else if (shuttleStatus == 2 || shuttleStatus == 12) {
          static uint8_t x = 128;
          static uint8_t flagx = 1;
          u8g2.drawBox(x, 40, 28, 5);
          u8g2.drawBox(x + 4, 45, 3, 2);
          u8g2.drawBox(x + 21, 45, 3, 2);

          if (flagx == 1 && x > 1) x--;
          else {
            flagx = 0;
            u8g2.drawBox(x, 34, 28, 3);
            u8g2.drawBox(x, 37, 3, 3);
            u8g2.drawBox(x + 13, 37, 3, 3);
            u8g2.drawBox(x + 25, 37, 3, 3);
            x++;
          }
          if (flagx == 0 && x == 127) {
            flagx = 1;
          }
        }
      } else if (page == MENU) {
        String shuttbacka = "";
        String fifolifo_modest = "";
        if (shuttbackaway) shuttbacka = "вкл ";
        else shuttbacka = "выкл";
        if (fifolifo_mode) fifolifo_modest = "LIFO ";
        else fifolifo_modest = "FIFO";
        strmenu[0] = " Подсчет палет: " + palletconfst;
        strmenu[1] = " Уплотнение";
        strmenu[2] = " Режим: " + fifolifo_modest;
        strmenu[3] = " Настройки";
        strmenu[4] = " Ошибки";
        strmenu[5] = " Доп. функции";
        strmenu[6] = " Назад";
        MenuOut();
      } else if (page == ERRORS) {
        errn = 0;
        warrn = 0;
        for (uint8_t i = 0; i < 16; i++) {
          if (bitRead(errorcode, i)) {
            errn++;
          }
        }
        //errn--;
        for (uint8_t ii = 0; ii < 8; ii++) {
          if (bitRead(warncode, ii)) {
            warrn++;
          }
        }
        if (errn) strmenu[0] = " Ошибки: " + String(errn - 1);
        else strmenu[0] = " Ошибки: 0";
        strmenu[1] = " Сброс ошибок";
        strmenu[2] = " Назад";
        strmenu[3] = "  ";
        strmenu[4] = "  ";
        MenuOut();
      } else if (page == UNLOAD_PALLETE) {
        quant = numquant1 * 10 + numquant2;
        Serial.println("Quant 1 = " + String(quant));
        u8g2.setCursor(0, 5 + 1 * 11);
        u8g2.print(" Выгрузить N палет:");
        u8g2.setFont(u8g2_font_9x15_t_cyrillic);
        u8g2.setCursor(0, 12 + 2 * 11);
        if (numquant1 == 0) u8g2.print(" 0" + String(quant));
        else u8g2.print(" " + String(quant));
        uint8_t sline;
        if (cursorPos == 3) sline = 24;
        else sline = 8;
        u8g2.drawBox(9 * cursorPos, 37, sline, 3);
      } else if (page == UNLOAD_PALLETE_SUCC) {
        u8g2.setCursor(0, 5 + 1 * 11);
        u8g2.print(" Выгрузка " + String(quant) + " палет");
        u8g2.setCursor(0, 5 + 2 * 11);
        u8g2.print(" запущена");
      } else if (page == UNLOAD_PALLETE_FAIL) {
        u8g2.setCursor(0, 5 + 1 * 11);
        u8g2.print(" Вы ввели " + String(quant));
        u8g2.setCursor(0, 5 + 2 * 11);
        u8g2.print(" отмена");
      } else if (page == OPTIONS) {
        String txtMode = " ";
        if (!fifolifo) {
          txtMode = ">>";
        } else {
          txtMode = "<<";
        }
        strmenu[0] = " МПР: " + String(mpr) + " mm";
        strmenu[1] = " Инверсия движения:" + txtMode;
        strmenu[2] = " Макс. скорость: " + String(speedset);
        strmenu[3] = " Изменить N уст-ва";
        strmenu[4] = " Защита батареи: " + String(lowbatt) + "%";
        strmenu[5] = " Инж. меню";
        strmenu[6] = " Сохр. параметров";
        strmenu[7] = " Назад";
        MenuOut();
      } else if (page == CHANGE_SHUTTLE_NUM) {
        //quant = numquant1 * 10 + numquant2;
        u8g2.setCursor(0, 5 + 1 * 11);
        u8g2.print(" Введите");
        u8g2.setCursor(0, 5 + 2 * 11);
        u8g2.print(" новый номер уст-ва");
        u8g2.setCursor(0, 5 + 3 * 11);
        u8g2.print(" " + String(shuttleTempNum));
      } else if (page == CHANGE_CHANNEL) {
        u8g2.setCursor(0, 5 + 1 * 11);
        u8g2.print(" Введите");
        u8g2.setCursor(0, 5 + 2 * 11);
        u8g2.print(" новый номер канала");
        u8g2.setCursor(0, 5 + 3 * 11);
        u8g2.print(" " + String(tempChannelNumber));
      } else if (page == ERRORS_LOG) {
        uint8_t ec;
        uint8_t epos = 1;
        for (ec = 1; ec < 16; ec++) {
          if (bitRead(errorcode, ec)) {
            u8g2.setCursor(0, 5 + epos * 11);
            u8g2.print(String(" E#0" + String(ec) + " " + ErrorMsgArray[ec]));
            epos++;
          }
        }
        if (epos == 1) {
          u8g2.setCursor(0, 16);
          u8g2.print("Нет ошибок");
        }
      } else if (page == ADDITIONAL_FUNCTIONS) {  //доп функции
        strmenu[0] = " Выгрузка N палет";
        strmenu[1] = " Вернуться в нач.поз.";
        strmenu[2] = " Диагностика";
        strmenu[3] = " Режим эвакуации: " + num_no_yes[evacuatstatus];
        strmenu[4] = " Статистика";
        strmenu[5] = " Защита меню: " + num_no_yes[pass_menu];
        strmenu[6] = " Обновление ПО ";
        strmenu[7] = " Назад";
        MenuOut();
      } else if (page == COUNT_START) {
        u8g2.setCursor(0, 5 + 1 * 11);
        u8g2.print(" Подсчет");
        u8g2.setCursor(0, 5 + 2 * 11);
        u8g2.print(" запущен");
      } else if (page == PACKING_CONTROL) {
        String tempcomp = " Уплотнение";
        strmenu[0] = tempcomp + " вперед";
        strmenu[1] = tempcomp + " назад";
        strmenu[2] = " Назад";
        strmenu[3] = " ";
        strmenu[4] = " ";
        MenuOut();
      } else if (page == WARNINGS_LOG) {
        u8g2.print(String(WarnMsgArray[warncode]));
      } else if (page == DIAGNOSTICS) {
        strmenu[0] = " Режим: " + String(testnum);
        strmenu[1] = " Timer1: " + String(testtimer1);
        strmenu[2] = " Timer2: " + String(testtimer2);
        strmenu[3] = " Повторы: " + String(testrepeat);
        strmenu[4] = " Старт";
        MenuOut();
      } else if (page == STATS) {
        strmenu[0] = " См.Загр " + String(statistic[0]);
        strmenu[1] = " См.Выгр " + String(statistic[1]);
        strmenu[2] = " См.Упл " + String(statistic[2]);
        strmenu[3] = " См.Подъем " + String(statistic[3]);
        strmenu[4] = " См.Пробег " + String(statistic[4]);
        strmenu[5] = " Выгр " + String(statistic[5]);
        strmenu[6] = " Загр " + String(statistic[6]);
        strmenu[7] = " Упл " + String(statistic[7]);
        strmenu[8] = " Подъем " + String(statistic[8]);
        strmenu[9] = " Пробег " + String(statistic[9]);
        MenuOut();
      } else if (page == BATTERY_PROTECTION) {
        String templv = String(minlevel);
        if (minlevel == 31) templv = "---";
        strmenu[0] = " Мин. заряд: " + templv;
        strmenu[1] = " Назад";
        strmenu[2] = " ";
        strmenu[3] = " ";
        strmenu[4] = " ";
        MenuOut();
      } else if (page == ENGINEERING_MENU) {
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
        strmenu[10] = " Назад";
        MenuOut();
      } else if (page == DEBUG_INFO && cursorPos == 1) {
        uint16_t sym_code = 0;
        u8g2.setFont(u8g2_font_6x13_t_cyrillic);
        u8g2.setCursor(2, 22);
        u8g2.print("Forw. chnl dist: " + String(sensor_channel_f));  // Канальный сенсор расстояния вперед
        u8g2.setCursor(2, 32);
        u8g2.print("Rvrs. chnl dist: " + String(sensor_channel_r));  // Канальный сенсор расстояния назад
        u8g2.setCursor(2, 42);
        u8g2.print("Forw. plt dist:  " + String(sensor_pallete_F));  // Паллетный сенсор расстояния вперед
        u8g2.setCursor(2, 52);
        u8g2.print("Rvrs. plt dist:  " + String(sensor_pallete_R));  // Паллетный сенсор расстояния назад
        u8g2.setCursor(2, 62);
        u8g2.print("Encoder ang: " + String(enc_mm));  // Показания энкодера



        u8g2.setFont(u8g2_font_unifont_t_symbols);
        static uint8_t count_timer = 0;
        if (SensorsDataTrig) count_timer++;
        if (count_timer > 3) {
          count_timer = 0;
        }
        sym_code = 0x25f7 - count_timer;
        u8g2.drawGlyph(120, 10, sym_code);
      } else if (page == DEBUG_INFO && cursorPos == 2) {
        uint16_t sym_code = 0;
        u8g2.setFont(u8g2_font_6x13_t_cyrillic);
        u8g2.setCursor(2, 22);
        u8g2.print(String(DATCHIK_F1) + " DATCHIK_F1");  // датчик определения паллет вперед 1
        u8g2.setCursor(2, 32);
        u8g2.print(String(DATCHIK_F2) + " DATCHIK_F2");  // датчик определения паллет вперед 2
        u8g2.setCursor(2, 42);
        u8g2.print(String(DATCHIK_R1) + " DATCHIK_R1");  // датчик определения паллет назад 1
        u8g2.setCursor(2, 52);
        u8g2.print(String(DATCHIK_R2) + " DATCHIK_R2");  // датчик определения паллет назад 2

        u8g2.setFont(u8g2_font_unifont_t_symbols);
        static uint8_t count_timer = 0;
        if (SensorsDataTrig) count_timer++;
        if (count_timer > 3) {
          count_timer = 0;
        }
        sym_code = 0x25f7 - count_timer;
        u8g2.drawGlyph(120, 10, sym_code);
      }

      else if (page == SYSTEM_SETTINGS_WARN) {
        u8g2.setCursor(40, 15);
        String passw = " ";
        if (cursorPos != 4) passw = "!";
        else passw = "*";
        u8g2.print("ВНИМАНИЕ" + passw);
        u8g2.setCursor(0, 26);
        u8g2.print("  Изменение   данных");
        u8g2.setCursor(0, 37);
        u8g2.print(" пар-в может привести");
        u8g2.setCursor(0, 48);
        u8g2.print("к повреждению устр-ва");
        u8g2.setCursor(0, 63);
        u8g2.print("< назад     вперед ОК");
      } else if (page == 21) {
        if (!UpdateParam) {
          strmenu[0] = " Считать пар-ры ";
        } else {
          strmenu[0] = " Пар-ры считаны";
        }
        strmenu[1] = " Номер шатт " + String(settDataMenu[0]);
        strmenu[2] = " Номер экр " + String(settDataMenu[1]);
        strmenu[3] = " away_set " + String(bitRead(settDataMenu[2], 0));
        strmenu[4] = " enc_invers " + String(bitRead(settDataMenu[2], 1));
        strmenu[5] = " slim " + String(bitRead(settDataMenu[2], 2));
        strmenu[6] = " d_canal_off " + String(bitRead(settDataMenu[2], 3));
        strmenu[7] = " away_filifo " + String(bitRead(settDataMenu[2], 4));
        strmenu[8] = " FIFO_MODE " + String(bitRead(settDataMenu[2], 5));
        strmenu[9] = " demo_en " + String(bitRead(settDataMenu[2], 6));
        strmenu[10] = " crane_stat " + String(bitRead(settDataMenu[2], 7));
        strmenu[11] = " stop_aft_pall " + String(dist_stop_cv3);
        strmenu[12] = " upl_wait " + String(upl_wait_time);
        strmenu[13] = " pall_plk " + String(pallet_plank);
        strmenu[14] = " pall_800 " + String(pallet_800_only);
        strmenu[15] = R_VERSION;
        strmenu[16] = " ";
        strmenu[17] = " Записать пар-ры";
        strmenu[18] = " Назад";
        MenuOut();
      } else if (page == STATUS_PGG) {
        strmenu[0] = " Параметры журналировпания";
        if (logWrite) strmenu[1] = " Журналирование: ВКЛ";
        else strmenu[1] = " Журналирование: ВЫКЛ";
        strmenu[2] = " Назад";
        strmenu[3] = "";
        strmenu[4] = "";
        strmenu[5] = "";
        MenuOut();
      } else if (page == PACKING_WARN) {
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
      } else if (page == UPDATE_FIRMWARE) {
        if (!isUpdateStarted) {
          WiFi.softAP(ssid, password, channel, hide_SSID, max_connection);
          server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(200, "text/plain", "Hi! I am ESP32.");
          });
          AsyncElegantOTA.begin(&server);  // Start ElegantOTA
          server.begin();
        }
        isUpdateStarted = true;
        u8g2.setCursor(0, 15);
        u8g2.print(" Точка доступа:");
        u8g2.setCursor(0, 28);
        u8g2.print(" esp32UPDATE");
        u8g2.setCursor(0, 39);
        u8g2.print(" IP адрес: ");
        u8g2.setCursor(0, 50);
        u8g2.print(" ");
        u8g2.print(WiFi.softAPIP());
        u8g2.setCursor(0, 63);
        u8g2.print("                  >ОК");

      } else if (page == MENU_PROTECTION) {
        temp_pin_code = temp_pin[0] * 1000 + temp_pin[1] * 100 + temp_pin[2] * 10 + temp_pin[3];
        u8g2.setCursor(0, 5 + 1 * 11);
        u8g2.print(" PIN CODE:");
        u8g2.setFont(u8g2_font_9x15_t_cyrillic);
        u8g2.setCursor(0, 12 + 2 * 11);
        String temp_z = "";
        if (temp_pin_code < 10) {
          u8g2.print(" 000" + String(temp_pin_code));
        } else if (temp_pin_code >= 10 && temp_pin_code < 100) {
          u8g2.print(" 00" + String(temp_pin_code));
        } else if (temp_pin_code >= 100 && temp_pin_code < 1000) {
          u8g2.print(" 000" + String(temp_pin_code));
        } else u8g2.print(" " + String(temp_pin_code));
        u8g2.drawBox(9 * cursorPos, 37, 8, 3);
      } else if (page == ACCESS_DENIED) {
        u8g2.setCursor(0, 5 + 1 * 11);
        u8g2.print(" Доступ");
        u8g2.setCursor(0, 5 + 2 * 11);
        u8g2.print(" запрещен");
      } else if (page == CALIBRATION) {
        strmenu[0] = " Выполнено:" + String(calibret);
        strmenu[1] = " Сохранить";
        strmenu[2] = " Назад";
        strmenu[3] = "";
        strmenu[4] = "";
        strmenu[5] = "";
        MenuOut();
      } else if (page == MOVEMENT) {
        strmenu[0] = " Движение >>";
        strmenu[1] = " Движение <<";
        strmenu[2] = " Назад";
        strmenu[3] = "";
        strmenu[4] = "";
        strmenu[5] = "";
        MenuOut();
      } else if (page == MOVEMENT_RIGHT) {
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
      } else if (page == MOVEMENT_LEFT) {
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
      if (page == UNLOAD_PALLETE_SUCC || page == UNLOAD_PALLETE_FAIL || page == ACCESS_DENIED) {
        delay(1200);
        cmdSend(16);
        page = MAIN;
      }
    } else delay(50);
    displayUpdate = false;
  }
  else digitalWrite(rfout0, HIGH);
}
#pragma region Функции...
void cmdSend(uint8_t numcmd) {
  int cnt;
  uint8_t oldtime = waittime;
  uint8_t mpro = 100 + mproffset;
  uint8_t chnlo = 100 + chnloffset;
  switch (numcmd) {
    case CMD_STOP:  //стоп
      manualMode = false;
      manualCommand = " ";
      Serial2.print(shuttnumst + "dStop_");
      delay(50);
      Serial2.print(shuttnumst + "dStop_");
      /*cnt = millis();
      while (inputString != shuttnumst + "dStop_!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
      if (inputString != shuttnumst + "dStop_!")
      {
        Serial2.print(shuttnumst + "dStop_");
        cnt = millis();
        while (inputString != shuttnumst + "dStop_!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
        if (inputString != shuttnumst + "dStop_!") Serial2.print(shuttnumst + "dStop_");
      }*/
      break;
    case CMD_STOP_MANUAL:  //отключение ручного режима
      manualMode = false;
      manualCommand = " ";
      Serial2.print(shuttnumst + "dStopM");
      delay(50);
      Serial2.print(shuttnumst + "dStopM");   
      break;
    case CMD_LOAD:  //загрузка
      manualMode = false;
      Serial2.print(shuttnumst + "dLoad_");
      cnt = millis();
      while (inputString != shuttnumst + "dLoad_!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
      if (inputString != shuttnumst + "dLoad_!")
      {
        Serial2.print(shuttnumst + "dLoad_");
        cnt = millis();
        while (inputString != shuttnumst + "dLoad_!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
        if (inputString != shuttnumst + "dLoad_!") Serial2.print(shuttnumst + "dLoad_");
      }
      break;
    case CMD_UNLOAD:  //выгрузка
      manualMode = false;
      Serial2.print(shuttnumst + "dUnld_");
      cnt = millis();
      while (inputString != shuttnumst + "dUnld_!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
      if (inputString != shuttnumst + "dUnld_!")
      {
        Serial2.print(shuttnumst + "dUnld_");
        cnt = millis();
        while (inputString != shuttnumst + "dUnld_!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
        if (inputString != shuttnumst + "dUnld_!") Serial2.print(shuttnumst + "dUnld_");
      }
      break;
    case CMD_CONT_LOAD:  //прод.загрузка
      manualMode = false;
      Serial2.print(shuttnumst + "dLLoad");
      cnt = millis();
      while (inputString != shuttnumst + "dLLoad!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
      if (inputString != shuttnumst + "dLLoad!")
      {
        Serial2.print(shuttnumst + "dLLoad");
        cnt = millis();
        while (inputString != shuttnumst + "dLLoad!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
        if (inputString != shuttnumst + "dLLoad!") Serial2.print(shuttnumst + "dLLoad");
      }
      break;
    case CMD_CONT_UNLOAD:  //прод.выгрузка
      manualMode = false;
      Serial2.print(shuttnumst + "dLUnld");
      cnt = millis();
      while (inputString != shuttnumst + "dLUnld!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
      if (inputString != shuttnumst + "dLUnld!")
      {
        Serial2.print(shuttnumst + "dLUnld");
        cnt = millis();
        while (inputString != shuttnumst + "dLUnld!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
        if (inputString != shuttnumst + "dLUnld!") Serial2.print(shuttnumst + "dLUnld");
      }
      break;
    case CMD_DEMO:  // прод.выгрузка
      manualMode = false;
      Serial2.print(shuttnumst + "dDemo_");
      cnt = millis();
      while (inputString != shuttnumst + "dDemo_!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
      if (inputString != shuttnumst + "dDemo_!")
      {
        Serial2.print(shuttnumst + "dDemo_");
        cnt = millis();
        while (inputString != shuttnumst + "dDemo_!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
        if (inputString != shuttnumst + "dDemo_!") Serial2.print(shuttnumst + "dDemo_");
      }
      break;
    case CMD_PLATFORM_LIFTING:  //подъем палатформы
      manualCommand = "/\\";
      Serial2.print(shuttnumst + "dUp___");
      /*cnt = millis();
      while (inputString != shuttnumst + "dUp___!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
      if (inputString != shuttnumst + "dUp___!")
      {
        Serial2.print(shuttnumst + "dUp___");
        cnt = millis();
        while (inputString != shuttnumst + "dUp___!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
        if (inputString != shuttnumst + "dUp___!") Serial2.print(shuttnumst + "dUp___");
      cnt = millis();
      }*/
      break;
    case CMD_PLATFORM_UNLIFTING:  //опускание платформы
      manualCommand = "\\/";
      Serial2.print(shuttnumst + "dDown_");
      /*cnt = millis();
      while (inputString != shuttnumst + "dDown_!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
      if (inputString != shuttnumst + "dDown_!")
      {
        Serial2.print(shuttnumst + "dDown_");
        cnt = millis();
        while (inputString != shuttnumst + "dDown_!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
        if (inputString != shuttnumst + "dDown_!") Serial2.print(shuttnumst + "dDown_");
      }*/
      break;
    case CMD_MOVEMENT_LEFT:  // движение влево
      manualCommand = "<<";
      Serial2.print(shuttnumst + "dLeft_");
      /*delay(15);
      cnt = millis();
      while (inputString != shuttnumst + "dLeft_!" && millis() - cnt < 250) {delay(100); Serial2.print(shuttnumst + "ngPing"); GetSerial2Data();}
      if (inputString != shuttnumst + "dLeft_!") Serial2.print(shuttnumst + "dLeft_");*/
      break;
    case CMD_MOVEMENT_RIGHT:  // движение вправо
      manualCommand = ">>";
      Serial2.print(shuttnumst + "dRight");
      /*cnt = millis();
      while (inputString != shuttnumst + "dRight!" && millis() - cnt < 250) {delay(100); Serial2.print(shuttnumst + "ngPing"); GetSerial2Data();}
      if (inputString != shuttnumst + "dRight!") Serial2.print(shuttnumst + "dRight");*/
      break;
    case CMD_REVERSE_ON:                     // инверсия движения
      Serial2.print(shuttnumst + "dRevOn");  //invert mode
      break;
    case CMD_REVERSE_OFF:                    // инверсия движения
      Serial2.print(shuttnumst + "dReOff");  //invert_mode
      break;
    case CMD_INTER_PALL_DISTANCE:  // выставление МПР
      if (mpr < 100) Serial2.print(shuttnumst + "dDm0" + String(mpr));
      else Serial2.print(shuttnumst + "dDm" + String(mpr));
      break;
    case CMD_UNLOAD_PALLET_BY_NUMBER:  // выгрузка заданного числа паллет
      if (quant < 10) {Serial2.print(shuttnumst + "dQt00" + String(quant)); Serial.println("Send CMD: " + shuttnumst + "dQt00" + String(quant));}
      else {Serial2.print(shuttnumst + "dQt0" + String(quant)); Serial.println("Send CMD: " + shuttnumst + "dQt0" + String(quant));}
      break;
    case 16:
      Serial2.print(shuttnumst + "dCharg");
      break;
    case 17:  // установка нового номера шаттла
      if (shuttleTempNum < 10) Serial2.print(shuttnumst + "dNN00" + String(shuttleTempNum));
      else Serial2.print(shuttnumst + "dNN0" + String(shuttleTempNum));
      break;
    case CMD_PING:  // запрос пинг
      Serial2.print(shuttnumst + "ngPing");
      break;
    case CMD_BACK_TO_ORIGIN:  // возврат в начальную позицию
      Serial2.print(shuttnumst + "dHome_");
      break;
    case CMD_SET_SPEED:  // установка скорости
      if (speedset < 10) Serial2.print(shuttnumst + "dSp00" + String(speedset));
      else if (speedset < 100) Serial2.print(shuttnumst + "dSp0" + String(speedset));
      else Serial2.print(shuttnumst + "dSp" + String(speedset));
      break;
    case CMD_SET_LENGTH:  // установка длинны шаттла
      if (shuttleLength == 800) Serial2.print(shuttnumst + "dSl080");
      else if (shuttleLength == 1000) Serial2.print(shuttnumst + "dSl100");
      else Serial2.print(shuttnumst + "dSl120");
      break;
    case CMD_GET_PARAM:  //
      Serial2.print(shuttnumst + "dSGet_");
      break;
    case CMD_EVAC:  //эвакуация
      if (evacuatstatus) Serial2.print(shuttnumst + "dEvOn_");
      else Serial2.print(shuttnumst + "dEvOff");
      break;
    case CMD_BATTERY_PROTECTION:  // отключение от разряда батареи
      if (lowbatt < 10) Serial2.print(shuttnumst + "dBc00" + String(lowbatt));
      else Serial2.print(shuttnumst + "dBc0" + String(lowbatt));
      break;
    case CMD_GET_ERRORS:  //ошибки
      Serial2.print(shuttnumst + "tError");
      delay(10);
      Serial2.print(shuttnumst + "dSGet_");
      break;
    case CMD_GET_MENU:
      Serial2.print(shuttnumst + "dGetPC");
      break;
    case CMD_PALLETE_COUNT:
      Serial2.print(shuttnumst + "dGetQu");
      break;
    case CMD_PACKING_BACK:  //уплотнение назад
      Serial2.print(shuttnumst + "dComBa");
      cnt = millis();
      while (inputString != shuttnumst + "dComBa!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
      if (inputString != shuttnumst + "dComBa!")
      {
        Serial2.print(shuttnumst + "dComBa");
        cnt = millis();
        while (inputString != shuttnumst + "dComBa!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
        if (inputString != shuttnumst + "dComBa!") Serial2.print(shuttnumst + "dComBa");
      }
      break;
    case CMD_PACKING_FORWARD:  //уплотнение вперед
      Serial2.print(shuttnumst + "dComFo");
      cnt = millis();
      while (inputString != shuttnumst + "dComFo!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
      if (inputString != shuttnumst + "dComFo!")
      {
        Serial2.print(shuttnumst + "dComFo");
        cnt = millis();
        while (inputString != shuttnumst + "dComFo!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
        if (inputString != shuttnumst + "dComFo!") Serial2.print(shuttnumst + "dComFo");
      }
      break;
    case CMD_WAIT_TIME:
      if (waittime < 10) Serial2.print(shuttnumst + "dWt0" + String(waittime));
      else Serial2.print(shuttnumst + "dWt" + String(waittime));
      break;
    case CMD_MPR_OFFSET:
      if (mpro < 10) Serial2.print(shuttnumst + "dMo00" + String(mpro));
      if (mpro < 100) Serial2.print(shuttnumst + "dMo0" + String(mpro));
      else Serial2.print(shuttnumst + "dMo" + String(mpro));
      break;
    case CMD_CHNL_OFFSET:
      if (chnlo < 10) Serial2.print(shuttnumst + "dMc00" + String(chnlo));
      if (chnlo < 100) Serial2.print(shuttnumst + "dMc0" + String(chnlo));
      else Serial2.print(shuttnumst + "dMc" + String(chnlo));
      break;
    case 32:
      Serial2.print(shuttnumst + "dTest2" + String(testtimer2));
      break;
    case 33:
      Serial2.print(shuttnumst + "dTestR" + String(testrepeat));
      break;
    case 34:
      Serial2.print(shuttnumst + "dTestG");
      break;
    case 35:
      Serial2.print(shuttnumst + "dPallQ");
      break;
    case 36:
      Serial2.print(shuttnumst + "dSttic" + String(odometr_pos));
      break;
    case 37:  //мин.заряд
      Serial2.print(shuttnumst + "dLevel" + String(minlevel));
      break;
    case 38:
      Serial2.print(shuttnumst + "dminLv");  //get min batt lvl
      break;
    case 39:
      Serial2.print(shuttnumst + "dSaveC");
      break;
    case CMD_FIFO_LIFO:
      if (fifolifo_mode) Serial2.print(shuttnumst + "dLIFO_");  //режим fifo/lifo
      else Serial2.print(shuttnumst + "dFIFO_");
      break;
    case 42:
      Serial2.print(shuttnumst + "dEngee");
      break;
    case 43:
      Serial2.print(shuttnumst + "dClbr_");  //Калибровка
      break;
    case 44:
      //Serial2.print(shuttnumst + "dGetLg");  // get all data
      //Serial2.print(shuttnumst + "dSGet_");  // get all data
      Serial2.print(shuttnumst + "dDataP");  // get all data
      break;
    case CMD_RESET:
      Serial2.print(shuttnumst + "dReset");
      cnt = millis();
      while (inputString != shuttnumst + "dReset!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
      if (inputString != shuttnumst + "dReset!")
      {
        Serial2.print(shuttnumst + "dReset");
        cnt = millis();
        while (inputString != shuttnumst + "dReset!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
        if (inputString != shuttnumst + "dReset!") Serial2.print(shuttnumst + "dReset");
      }
      break;
    case CMD_MANUAL:
      Serial2.print(shuttnumst + "dManua");
      cnt = millis();
      while (inputString != shuttnumst + "dManua!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
      if (inputString != shuttnumst + "dManua!")
      {
        Serial2.print(shuttnumst + "dManua");
        cnt = millis();
        while (inputString != shuttnumst + "dManua!" && millis() - cnt < 500) {delay(20); GetSerial2Data();}
        if (inputString != shuttnumst + "dManua!") Serial2.print(shuttnumst + "dManua");
      }
      break;
    case 46:
      String tempsnum = shuttnumst;
      uint16_t *p_shuttnum;
      uint16_t *p_DataSend;
      p_shuttnum = (uint16_t *)&tempsnum;
      p_DataSend = (uint16_t *)&DataSend[0];
      *p_DataSend = *p_shuttnum;
      DataSend[2] = 68;  //D
      DataSend[3] = 84;  //T
      DataSend[4] = settDataMenu[1];
      DataSend[5] = settDataMenu[2];
      uint8_t rever[4];
      uint32_t *p_val = (uint32_t *)&rever[0];
      *p_val = *(&dist_stop_cv3);
      uint8_t i;
      for (i = 0; i < 4; i++) {
        DataSend[i + 6] = rever[3 - i];
      }
      *p_val = *(&upl_wait_time);
      for (i = 0; i < 4; i++) {
        DataSend[i + 10] = rever[3 - i];
      }
      DataSend[15] = pallet_plank;
      DataSend[16] ^= (-pallet_800_only ^ DataSend[16]) & (1 << 0);
      Serial2.write(DataSend, 20);
      break;
      /*case 47:
      Serial2.print(shuttnumst+"dmprcalRev" + "dCharg"); //расстояние начала срабатывания датчика палеты мпр
      break;*/
  }
}
//заряд
int getVoltage() {
  int raw = analogRead(Battery_Pin);
  int volt = map(raw, 740, 1100, 0, 100);
  if (volt < 0) {
    volt = 0;
  } else if (volt > 100) {
    volt = 100;
  }
  //volt = volt / 100;
  //Serial2.print("A0 "); Serial2.println(raw);
  //Serial2.print("Voltage "); Serial2.println(volt);
  return volt;
}


void keypadEvent(KeypadEvent key) {
  switch (kpd.getState()) {

    case PRESSED:

      //beep1KHz();
      currentKey = key;
      buttonTimer = millis();
      displayOffTimer = millis();
      mpingtime = millis();
      buttonActive = true;
      displayOffInterval = 900000;
      //Serial2.println(key);

      if (displayActive) {
        if (key == '6') {
          cmdSend(CMD_STOP);
        } else if (key == 'D' && hideshuttnum) {
          shuttleNumber = shuttleTempNum;
          EEPROM.begin(4);
          EEPROM.write(addr0, shuttleNumber);
          EEPROM.end();
          newMpr = 0;
          quant = 0;
          newshuttleLength = 0;
          newspeedset = 0;
          newlowbatt = 100;
          warncode = 0;
          newreverse = 2;
          waittime = 0;
          newwaittime = 0;
          mproffset = 0;
          newmproffset = 120;
          chnloffset = 0;
          newchnloffset = 120;
          delay(300);
          shuttnumst = shuttnum[shuttleNumber - 1];
          cmdSend(16);
          hideshuttnum = 0;
        }
        if (page == MAIN) {
          switch (key) {
            case '8':
              cmdSend(CMD_MOVEMENT_RIGHT);
              break;
            case '0':
              cmdSend(CMD_MOVEMENT_LEFT);
              break;
            case '9':
              cmdSend(CMD_PLATFORM_UNLIFTING);
              break;
            case 'E':
              cmdSend(CMD_PLATFORM_LIFTING);
              break;
            case '1':
              //shuttleNumber=1;
              break;
            case '2':
              //shuttleNumber=2;
              break;
            case '3':
              //shuttleNumber=3;
              break;
          }
        } else if (page == CHANGE_SHUTTLE_NUM) {
          if (key == 'A') {
            if (shuttleTempNum < 26) shuttleTempNum++;
          } else if (key == 'B') {
            if (shuttleTempNum > 1) shuttleTempNum--;
          } else if (key == 'D') {
            shuttleNumber = shuttleTempNum;
            shuttnewnumst = shuttnum[shuttleTempNum - 1];
            cmdSend(17);
            EEPROM.begin(4);
            EEPROM.write(addr0, shuttleNumber);
            EEPROM.end();
            newMpr = 0;
            quant = 0;
            newshuttleLength = 0;
            newspeedset = 0;
            newlowbatt = 100;
            warncode = 0;
            newreverse = 2;
            waittime = 0;
            newwaittime = 0;
            mproffset = 0;
            newmproffset = 120;
            chnloffset = 0;
            newchnloffset = 120;
            delay(300);
            //cmdStatus();
            cmdSend(16);
            page = MAIN;  // toggle menu mode
            cursorPos = 1;
          }
        } else if (page == CHANGE_CHANNEL) {
          if (key == 'A') {
            if (tempChannelNumber < 100) tempChannelNumber++;
          } else if (key == 'B') {
            if (tempChannelNumber > 1) tempChannelNumber--;
          } else if (key == 'D' && tempChannelNumber != channelNumber) {
            channelNumber = tempChannelNumber;
            configArray[4] = channelNumber;
            Serial2.write(configArray, sizeof(configArray));
            EEPROM.begin(10);
            for (uint8_t i = 0; i < sizeof(configArray); i++) {
              if (EEPROM.read(configArrayStartAddress + i) != configArray[i]) {
                EEPROM.write(configArrayStartAddress + i, configArray[i]);
              }
            }
            EEPROM.write(configArrayStartAddress + 4, channelNumber);
            EEPROM.end();
            isFabala = true;
            delay(300);
            //cmdStatus();
            cmdSend(16);
            page = MAIN;  // toggle menu mode
            cursorPos = 1;
          }
        }
      } else {
        cmdSend(16);
        countcharge = 0;
        getchargetime = millis();
      }
      break;

    case RELEASED:
      buttonActive = false;
      currentKey = ' ';
      buttonTimer = 0;
      displayUpdate = true;
      if (displayActive && dispactivate) {
        //Buffer[0]=0;
        //Client.WriteArea(S7AreaMK, 0, 360, 1, &Buffer); // Stop manual movements
        switch (key) {
          case 'D':  // Long press OK - Demo mode
            if (longPressActive) {
              if (page == MAIN) {
                cmdSend(CMD_DEMO);
              }
              //shuttleStatus=5;
            } else if (page == MAIN) {
              cmdSend(16);
              countcharge = 0;
              getchargetime = millis();
            } else {
              if (page == MENU && cursorPos == 5) {
                page = ERRORS;
                cursorPos = 1;
                cmdSend(CMD_GET_ERRORS);
                countcharge = 0;
                getchargetime = millis();
              } else if (page == MENU && cursorPos == 2) {
                page = PACKING_WARN;
                //cursorPos = 1;
              } else if (page == PACKING_WARN && cursorPos == 2) {
                if (pass_menu == 0) {
                  cursorPos = 1;
                  page = PACKING_CONTROL;
                } else {
                  cursorPos = 1;
                  pageAfterPin = PACKING_CONTROL;
                  page = MENU_PROTECTION;
                }
              } else if (page == UPDATE_FIRMWARE) {
                page = MAIN;
                cursorPos = 1;
              } else if (page == MENU && cursorPos == 6) {
                page = ADDITIONAL_FUNCTIONS;
                cursorPos = 1;
                countcharge = 0;
                getchargetime = millis();
              } else if (page == MENU && cursorPos == 4) {
                if (pass_menu == 0) {
                  page = OPTIONS;
                  cursorPos = 1;
                  cmdSend(CMD_GET_PARAM);
                  countcharge = 0;
                  getchargetime = millis();
                } else {
                  pageAfterPin = OPTIONS;
                  cursorPos = 1;
                  page = MENU_PROTECTION;
                }
              } else if (page == MENU && cursorPos == 7) {
                page = MAIN;
                cursorPos = 1;
              } else if (page == ADDITIONAL_FUNCTIONS && cursorPos == 1) {
                page = UNLOAD_PALLETE;
                cursorPos = 1;
              } else if (page == ADDITIONAL_FUNCTIONS && cursorPos == 7) {
                if (pass_menu == 0) {
                  page = UPDATE_FIRMWARE;
                } else {
                  pageAfterPin = UPDATE_FIRMWARE;
                  cursorPos = 1;
                  page = MENU_PROTECTION;
                }
              } else if (page == ADDITIONAL_FUNCTIONS && cursorPos == 8) {
                page = MENU;
                cursorPos = 1;
              } else if (page == ERRORS && cursorPos == 1) {
                page = ERRORS_LOG;
              } else if (page == ERRORS && cursorPos == 3) {
                page = MENU;
                cursorPos = 1;
              } else if (page == ADDITIONAL_FUNCTIONS && cursorPos == 3) {
                page = DIAGNOSTICS;
                cursorPos = 1;
                cmdSend(34);
                countcharge = 0;
                getchargetime = millis();
              } else if (page == ADDITIONAL_FUNCTIONS && cursorPos == 7) {
                page = MENU;
                cursorPos = 1;
              } else if (page == ERRORS && cursorPos == 2) {
                cmdSend(CMD_RESET);
              } else if (page == OPTIONS && cursorPos == 4) {
                page = CHANGE_SHUTTLE_NUM;
              } else if (page == OPTIONS && cursorPos == 7) {
                cmdSend(39);
              } else if (page == OPTIONS && cursorPos == 6) {
                page = ENGINEERING_MENU;
                cursorPos = 1;
                cmdSend(42);
              } else if (page == OPTIONS && cursorPos == 8) {
                page = MENU;
                cursorPos = 1;
              } else if (page == UNLOAD_PALLETE) {
                if (quant > 0) {
                  if (shuttleStatus != 13) {shuttleStatus = 13; cmdSend(CMD_UNLOAD_PALLET_BY_NUMBER);}
                  page = UNLOAD_PALLETE_SUCC;
                } else page = UNLOAD_PALLETE_FAIL;
              } else if (page == ADDITIONAL_FUNCTIONS && cursorPos == 2) {
                cmdSend(CMD_BACK_TO_ORIGIN);
              }
              //else if (page==11 && cursorPos==3) {
              //  cmdEvacuation();
              //}
              else if (page == ADDITIONAL_FUNCTIONS && cursorPos == 5) {
                page = STATS;
                cursorPos = 1;
                for (uint8_t i = 0; i < 10; i++) {
                  statistic[i] = 0;
                }
                //countcharge = 0;
                odometr_pos = 0;
                countcharge = 0;
                getchargetime = millis();
                cmdSend(36);
              } else if (page == MENU && cursorPos == 1) {
                palletconfst = ">>>";
                cmdSend(CMD_PALLETE_COUNT);
              } else if (page == PACKING_CONTROL && cursorPos == 1) {
                cmdSend(CMD_PACKING_FORWARD);
                page = MAIN;
                cursorPos = 1;
              } else if (page == PACKING_CONTROL && cursorPos == 2) {
                cmdSend(CMD_PACKING_BACK);
                page = MAIN;
                cursorPos = 1;
              } else if (page == PACKING_CONTROL && cursorPos == 3) {
                page = MENU;
                cursorPos = 1;
              } else if (page == BATTERY_PROTECTION && cursorPos == 2) {
                page = ERRORS;
                cursorPos = 1;
              } else if (page == ERRORS_LOG || page == WARNINGS_LOG) page = ERRORS;
              else if (page == DIAGNOSTICS && cursorPos == 5) {
                cmdSend(29);
                delay(300);
                page = MAIN;
                cursorPos = 1;
              } else if (page == ENGINEERING_MENU) {
                if (cursorPos == 1) {
                  page = CALIBRATION;
                  cursorPos = 1;
                } else if (cursorPos == 2) {
                  page = DEBUG_INFO;
                  cursorPos = 1;
                  SensorsDataTrig = false;
                } else if (cursorPos == 3) {
                  page = SYSTEM_SETTINGS_WARN;
                  cursorPos = 1;
                } else if (cursorPos == 4) {
                  page = STATUS_PGG;
                  cursorPos = 1;
                  cmdSend(44);
                } else if (cursorPos == 5) {
                  page = MOVEMENT;
                  cursorPos = 1;
                } else if (cursorPos == 6) {
                  page = CHANGE_CHANNEL;
                  cursorPos = 1;
                } else if (cursorPos == 11) {
                  page = OPTIONS;
                  cursorPos = 1;
                }
              } else if (page == SYSTEM_SETTINGS_WARN && cursorPos == 4) {
                page = STATUS_PGG;
                cursorPos = 1;
                cmdSend(44);
              } else if (page == DEBUG_INFO) {
                SensorsDataTrig = !SensorsDataTrig;
              } else if (page == STATUS_PGG && cursorPos == 3) {
                cursorPos = 1;
                page = ENGINEERING_MENU;
              } else if (page == STATUS_PGG) {
                cmdSend(44);
              } else if (page == MENU_PROTECTION) {
                if (pin_code == temp_pin_code) {
                  if (pass_menu_trig == 0) {
                    page = pageAfterPin;
                    cursorPos = 1;
                  } else {
                    pass_menu_trig = 0;
                    if (pass_menu) pass_menu = 0;
                    else pass_menu = 1;
                    EEPROM.begin(4);
                    EEPROM.write(addr0 + 1, pass_menu);
                    EEPROM.end();
                    delay(50);
                    page = ADDITIONAL_FUNCTIONS;
                    cursorPos = 1;
                  }
                } else page = ACCESS_DENIED;
                for (int8_t i = 0; i < 4; i++) {
                  temp_pin[i] = 0;
                }
              } else if (page == CALIBRATION) {
                if (cursorPos == 1) {
                  cmdSend(43);
                } else if (cursorPos == 2) {
                  cmdSend(39);
                } else if (cursorPos == 3) {
                  page = ENGINEERING_MENU;
                  cursorPos = 1;
                }
              } else if (page == MOVEMENT) {
                if (cursorPos == 1) {
                  page = MOVEMENT_RIGHT;
                  cursorPos = 1;
                } else if (cursorPos == 2) {
                  page = MOVEMENT_LEFT;
                  cursorPos = 1;
                } else if (cursorPos == 3) {
                  page = ENGINEERING_MENU;
                  cursorPos = 1;
                }
              } else if (page == MOVEMENT_RIGHT) {
                if (cursorPos == 1) {
                  Serial2.print(shuttnumst + "dMf010");
                } else if (cursorPos == 2) {
                  Serial2.print(shuttnumst + "dMf020");
                } else if (cursorPos == 3) {
                  Serial2.print(shuttnumst + "dMf030");
                } else if (cursorPos == 4) {
                  Serial2.print(shuttnumst + "dMf050");
                } else if (cursorPos == 5) {
                  Serial2.print(shuttnumst + "dMf100");
                } else if (cursorPos == 6) {
                  Serial2.print(shuttnumst + "dMf200");
                } else if (cursorPos == 7) {
                  Serial2.print(shuttnumst + "dMf300");
                } else if (cursorPos == 8) {
                  Serial2.print(shuttnumst + "dMf500");
                } else if (cursorPos == 9) {
                  page = MOVEMENT;
                  cursorPos = 1;
                }
              } else if (page == MOVEMENT_LEFT) {
                if (cursorPos == 1) {
                  Serial2.print(shuttnumst + "dMr010");
                } else if (cursorPos == 2) {
                  Serial2.print(shuttnumst + "dMr020");
                } else if (cursorPos == 3) {
                  Serial2.print(shuttnumst + "dMr030");
                } else if (cursorPos == 4) {
                  Serial2.print(shuttnumst + "dMr050");
                } else if (cursorPos == 5) {
                  Serial2.print(shuttnumst + "dMr100");
                } else if (cursorPos == 6) {
                  Serial2.print(shuttnumst + "dMr200");
                } else if (cursorPos == 7) {
                  Serial2.print(shuttnumst + "dMr300");
                } else if (cursorPos == 8) {
                  Serial2.print(shuttnumst + "dMr500");
                } else if (cursorPos == 9) {
                  page = MOVEMENT;
                  cursorPos = 1;
                }
              }
            }
            break;
          case '7':
            if (page == MAIN) {

              page = MENU;
              //cmdGetData();
              cmdSend(CMD_GET_MENU);
              countcharge = 0;
              getchargetime = millis();

            } else {
              if (page == ADDITIONAL_FUNCTIONS || page == MENU_PROTECTION) pass_menu_trig = 0;
              if (page == PACKING_WARN) page = MENU;
              if (page == CHANGE_CHANNEL) {
                tempChannelNumber = channelNumber;
                page = MAIN;
              } else if (page == SYSTEM_SETTINGS_WARN) page = ENGINEERING_MENU;
              else page = MAIN;
              cmdSend(16);
              countcharge = 0;
              getchargetime = millis();
            }
            cursorPos = 1;
            break;
          case '8':
          if (!manualMode && longPressActive) {
              cmdSend(CMD_MOVEMENT_RIGHT);
            } else if (page == MAIN && manualMode) {
              cmdSend(CMD_STOP_MANUAL);
            } else if (page == MENU || page == ERRORS || page == OPTIONS
                       || page == ADDITIONAL_FUNCTIONS || page == PACKING_CONTROL
                       || page == DIAGNOSTICS || page == STATS || page == BATTERY_PROTECTION
                       || page == ENGINEERING_MENU) {
              cursorPos--;
              if (cursorPos < 1) {
                if (page == MENU) cursorPos = 7;
                else if (page == ADDITIONAL_FUNCTIONS) cursorPos = 8;
                else if (page == OPTIONS) cursorPos = 8;
                else if (page == ENGINEERING_MENU) cursorPos = 11;
                else if (page == PACKING_CONTROL) cursorPos = 3;
                else if (page == BATTERY_PROTECTION) cursorPos = 2;
                else if (page == ERRORS) cursorPos = 3;
                else cursorPos = 6;
              }
            } else if (page == UNLOAD_PALLETE) {
              if (cursorPos == 1) {
                numquant1++;
                if (numquant1 > 9) numquant1 = 0;
              } else if (cursorPos == 2) {
                numquant2++;
                if (numquant2 > 9) numquant2 = 0;
              }
            } else if (page == MENU_PROTECTION) {
              temp_pin[cursorPos - 1]++;
              if (temp_pin[cursorPos - 1] > 9) temp_pin[cursorPos - 1] = 0;
            } else if (page == STATUS_PGG) {
              cursorPos--;
              if (cursorPos < 1) cursorPos = 3;
            } else if (page == DEBUG_INFO) {
              cursorPos--;
              if (cursorPos < 1) cursorPos = 1;
            } else if (page == SYSTEM_SETTINGS_WARN) {
              cursorPos++;
              if (cursorPos > 8) cursorPos = 1;
            } else if (page == CALIBRATION) {
              cursorPos--;
              if (cursorPos < 1) cursorPos = 3;
            } else if (page == MOVEMENT) {
              cursorPos--;
              if (cursorPos < 1) cursorPos = 3;
            } else if (page == MOVEMENT_RIGHT) {
              cursorPos--;
              if (cursorPos < 1) cursorPos = 9;
            } else if (page == MOVEMENT_LEFT) {
              cursorPos--;
              if (cursorPos < 1) cursorPos = 9;
            }

            break;
          case '0':
          if (!manualMode && longPressActive) {
              cmdSend(CMD_MOVEMENT_LEFT);
            } else if (page == MAIN && manualMode) {
              cmdSend(CMD_STOP_MANUAL);
            } else if (page == MENU || page == ERRORS || page == ADDITIONAL_FUNCTIONS
                       || page == PACKING_CONTROL || page == DIAGNOSTICS
                       || page == STATS || page == BATTERY_PROTECTION) {
              cursorPos++;
              if ((cursorPos > 6 && page != MENU && page != STATS && page != ADDITIONAL_FUNCTIONS) || (cursorPos > 3 && page == PACKING_CONTROL)) cursorPos = 1;
              else if (page == MENU) {
                if (cursorPos > 7) cursorPos = 1;
              } else if (page == ADDITIONAL_FUNCTIONS && cursorPos > 8) {
                cursorPos = 1;
              } else if (page == STATS && cursorPos > 10) cursorPos = 10;
              else if (page == BATTERY_PROTECTION && cursorPos > 2) cursorPos = 1;
              else if (page == ERRORS && cursorPos > 3) cursorPos = 1;
              else if (page == MENU && cursorPos > 7) cursorPos = 1;
            } else if (page == CALIBRATION) {
              cursorPos++;
              if (cursorPos > 3) cursorPos = 1;
            } else if (page == MOVEMENT) {
              cursorPos++;
              if (cursorPos > 3) cursorPos = 1;
            } else if (page == OPTIONS) {
              cursorPos++;
              if (cursorPos > 8) cursorPos = 1;
            } else if (page == ENGINEERING_MENU) {
              cursorPos++;
              if (cursorPos > 11) cursorPos = 1;
            } else if (page == UNLOAD_PALLETE) {
              if (cursorPos == 1) {
                numquant1--;
                if (numquant1 < 0) numquant1 = 9;
              } else if (cursorPos == 2) {
                numquant2--;
                if (numquant2 < 0) numquant2 = 9;
              }
            } else if (page == MENU_PROTECTION) {
              temp_pin[cursorPos - 1]--;
              if (temp_pin[cursorPos - 1] < 0) temp_pin[cursorPos - 1] = 9;
            } else if (page == STATUS_PGG) {
              cursorPos++;
              if (cursorPos > 3) cursorPos = 1;
            } else if (page == DEBUG_INFO) {
              cursorPos++;
              if (cursorPos > 3) cursorPos = 3;
            } else if (page == MOVEMENT_RIGHT) {
              cursorPos++;
              if (cursorPos > 9) cursorPos = 1;
            } else if (page == MOVEMENT_LEFT) {
              cursorPos++;
              if (cursorPos > 9) cursorPos = 1;
            }
            break;
          case '9':
            if (page == OPTIONS && cursorPos == 2) {
              fifolifo = !fifolifo;
              newreverse = fifolifo;
              if (fifolifo) cmdSend(CMD_REVERSE_OFF);
              else cmdSend(CMD_REVERSE_ON);
            } else if (page == OPTIONS && cursorPos == 1) {  // MPR--
              mpr -= 10;
              if (mpr < 50) mpr = 50;
              newMpr = mpr;
              cmdSend(CMD_INTER_PALL_DISTANCE);
            } else if (page == UNLOAD_PALLETE) {
              cursorPos--;
              if (cursorPos < 1) cursorPos = 2;
            } else if (page == MENU_PROTECTION) {
              cursorPos--;
              if (cursorPos < 1) cursorPos = 4;
            } else if (page == MAIN) {
              manualCommand = " ";
            } else if (page == OPTIONS && cursorPos == 3) {
              speedset--;
              if (speedset < 3) speedset = 3;
              newspeedset = speedset;
              cmdSend(CMD_SET_SPEED);
            } else if (page == ENGINEERING_MENU && cursorPos == 7) {
              shuttleLength -= 200;
              if (shuttleLength < 800) shuttleLength = 1200;
              newshuttleLength = shuttleLength;
              cmdSend(CMD_SET_LENGTH);
            } else if (page == ENGINEERING_MENU && cursorPos == 8) {
              waittime--;
              if (waittime < 5) waittime = 5;
              else if (waittime > 30) waittime = 30;
              newwaittime = waittime;
              cmdSend(CMD_WAIT_TIME);
            } else if (page == ENGINEERING_MENU && cursorPos == 9) {
              mproffset -= 10;
              if (mproffset < -100) mproffset = -100;
              else if (mproffset > 100) mproffset = 100;
              newmproffset = mproffset;
              cmdSend(CMD_MPR_OFFSET);
            } else if (page == ENGINEERING_MENU && cursorPos == 10) {
              chnloffset -= 10;
              if (chnloffset < -100) chnloffset = -100;
              else if (chnloffset > 100) chnloffset = 100;
              newchnloffset = chnloffset;
              cmdSend(CMD_CHNL_OFFSET);
            } else if (page == BATTERY_PROTECTION && cursorPos == 1) {
              minlevel -= 1;
              if (minlevel < 1) minlevel = 0;
              cmdSend(37);
            } else if (page == ADDITIONAL_FUNCTIONS && cursorPos == 4) {
              if (evacuatstatus) evacuatstatus = 0;
              else evacuatstatus = 1;
              cmdSend(CMD_EVAC);
            } else if (page == ADDITIONAL_FUNCTIONS && cursorPos == 6) {
              pass_menu_trig = 1;
              page = MENU_PROTECTION;
              cursorPos = 1;
            } else if (page == OPTIONS && cursorPos == 5) {
              lowbatt--;
              if (lowbatt < 0) lowbatt = 0;
              newlowbatt = lowbatt;
              cmdSend(CMD_BATTERY_PROTECTION);
            } else if (page == DIAGNOSTICS && cursorPos == 1) {
              testnum--;
              if (testnum < 1) testnum = 1;
              cmdSend(30);
            } else if (page == DIAGNOSTICS && cursorPos == 2) {
              if (testtimer1 <= 15) testtimer1--;
              else testtimer1 -= 5;
              if (testtimer1 < 0) testtimer1 = 90;
              cmdSend(31);
            } else if (page == DIAGNOSTICS && cursorPos == 3) {
              if (testtimer2 <= 15) testtimer2--;
              else testtimer2 -= 5;
              if (testtimer2 < 0) testtimer2 = 90;
              cmdSend(32);
            } else if (page == DIAGNOSTICS && cursorPos == 4) {
              if (testrepeat <= 20) testrepeat--;
              else if (testrepeat <= 190) testrepeat -= 10;
              else if (testrepeat <= 990) testrepeat -= 50;
              else testrepeat -= 500;
              if (testrepeat < 1) testrepeat = 19990;
              cmdSend(33);
            } else if (page == STATUS_PGG && cursorPos == 2) {
              logWrite = !logWrite;
              Serial2.print(shuttnumst + "dLg" + logWrite);
              cmdSend(44);
            }
            break;
          case 'E':
            if ((page == OPTIONS && cursorPos == 2)) {
              fifolifo = !fifolifo;
              newreverse = fifolifo;
              if (fifolifo) cmdSend(CMD_REVERSE_OFF);
              else cmdSend(CMD_REVERSE_ON);
            } else if (page == OPTIONS && cursorPos == 1) {
              mpr += 10;
              if (mpr > 390) mpr = 400;
              newMpr = mpr;
              cmdSend(CMD_INTER_PALL_DISTANCE);
            } else if (page == MAIN) {
              manualCommand = " ";
            } else if (page == PACKING_CONTROL) {
              cursorPos++;
              if (cursorPos > 2) cursorPos = 1;
            } else if (page == MENU_PROTECTION) {
              cursorPos++;
              if (cursorPos > 4) cursorPos = 1;
            } else if (page == OPTIONS && cursorPos == 3) {
              speedset++;
              if (speedset > 100) speedset = 100;
              newspeedset = speedset;
              cmdSend(CMD_SET_SPEED);
            } else if (page == ENGINEERING_MENU && cursorPos == 7) {
              shuttleLength += 200;
              if (shuttleLength > 1200) shuttleLength = 800;
              newshuttleLength = shuttleLength;
              cmdSend(CMD_SET_LENGTH);
            } else if (page == ENGINEERING_MENU && cursorPos == 8) {
              waittime++;
              if (waittime > 30) waittime = 30;
              else if (waittime < 5) waittime = 5;
              newwaittime = waittime;
              cmdSend(CMD_WAIT_TIME);
            } else if (page == ENGINEERING_MENU && cursorPos == 9) {
              mproffset += 10;
              if (mproffset > 100) mproffset = 100;
              else if (mproffset < -100) mproffset = -100;
              newmproffset = mproffset;
              cmdSend(CMD_MPR_OFFSET);
            } else if (page == ENGINEERING_MENU && cursorPos == 10) {
              chnloffset += 10;
              if (chnloffset > 100) chnloffset = 100;
              else if (chnloffset < -100) chnloffset = -100;
              newchnloffset = chnloffset;
              cmdSend(CMD_CHNL_OFFSET);
            } else if (page == ADDITIONAL_FUNCTIONS && cursorPos == 4) {
              if (evacuatstatus) evacuatstatus = 0;
              else evacuatstatus = 1;
              cmdSend(CMD_EVAC);
            } else if (page == ADDITIONAL_FUNCTIONS && cursorPos == 6) {
              pass_menu_trig = 1;
              page = MENU_PROTECTION;
              cursorPos = 1;
            } else if (page == OPTIONS && cursorPos == 5) {
              lowbatt++;
              if (lowbatt > 50) lowbatt = 50;
              newlowbatt = lowbatt;
              cmdSend(CMD_BATTERY_PROTECTION);
            } else if (page == BATTERY_PROTECTION && cursorPos == 1) {
              minlevel += 1;
              if (minlevel > 29) minlevel = 30;
              cmdSend(37);
            } else if (page == DIAGNOSTICS && cursorPos == 1) {
              testnum++;
              if (testnum > 4) testnum = 4;
              cmdSend(30);
            } else if (page == DIAGNOSTICS && cursorPos == 2) {
              if (testtimer1 >= 15) testtimer1 += 5;
              else testtimer1++;
              if (testtimer1 > 90) testtimer1 = 0;
              cmdSend(31);
            } else if (page == DIAGNOSTICS && cursorPos == 3) {
              if (testtimer2 >= 15) testtimer2 += 5;
              else testtimer2++;
              if (testtimer2 > 90) testtimer2 = 0;
              cmdSend(32);
            } else if (page == DIAGNOSTICS && cursorPos == 4) {
              if (testrepeat >= 990) testrepeat += 500;
              else if (testrepeat >= 190) testrepeat += 50;
              else if (testrepeat >= 20) testrepeat += 5;
              else testrepeat++;
              if (testrepeat > 19990) testrepeat = 1;
              cmdSend(33);
            } else if (page == MENU && cursorPos == 3) {  // режим fifo/lifo
              fifolifo_mode = !fifolifo_mode;
              cmdSend(CMD_FIFO_LIFO);
            } else if (page == STATUS_PGG && cursorPos == 2) {
              logWrite = !logWrite;
              Serial2.print(shuttnumst + "dLg" + logWrite);
              cmdSend(44);
            }
            break;
          case '5':
            if (page == MAIN) {
              if (longPressActive) {
                cmdSend(CMD_CONT_LOAD);
                //shuttleStatus=5;
              } else cmdSend(CMD_LOAD);
            }
            break;
          case 'C':
            // if (longPressActive)
            //  shuttleStatus = 7;else
            //  shuttleStatus=2;
            if (page == MAIN) {
              if (longPressActive) {
                cmdSend(CMD_CONT_UNLOAD);
                //shuttleStatus=5;
              } else if (shuttleStatus == 10) {quant++; uctimer = millis();}
            }
            break;
          case '6':
            //  shuttleStatus=0;
            if (!manualMode && longPressActive) {
              cmdSend(CMD_MANUAL);
              manualMode = true;
            }
            break;
        }
        if (key == '1' || key == '2' || key == '3' || key == '4' || key == 'A' || key == 'B') {
          if (page == MAIN) {
            int Tempshutnum = 1;
            shuttnumOffInterval = millis();
            if (key == 'A') {
              if (!hideshuttnum && shuttleNumber <= 26) {
                shuttleTempNum = shuttleNumber + 1;
                if (shuttleTempNum > 26) shuttleTempNum = 26;
              } else if (shuttleTempNum < 26) shuttleTempNum++;
              Tempshutnum = shuttleTempNum;
            } else if (key == 'B') {
              if (!hideshuttnum && shuttleNumber >= 1) {
                shuttleTempNum = shuttleNumber - 1;
                if (shuttleTempNum < 1) shuttleTempNum = 1;
              } else if (shuttleTempNum > 1) shuttleTempNum--;
              Tempshutnum = shuttleTempNum;
            } else {
              Tempshutnum = int(key) - 48;
              shuttleTempNum = Tempshutnum;
            }
            hideshuttnum = true;
            Serial2.print(shuttnum[Tempshutnum - 1] + "dBeep_");
          }
        }
      }
      dispactivate = 1;
      break;
    case HOLD:
      displayOffTimer = millis();
    default:
      break;
  }
}
void PinSetpcf() {
  for (uint8_t i = 0; i < 8; i++) {
    if (i > 3 && i < 7) {
      kpd.pin_mode(i, OUTPUT);
      kpd.pin_write(i, LOW);
    } else {
      kpd.pin_mode(i, INPUT_PULLUP);
    }
  }
}
void MarkTime() {
  Elapsed = millis();
}
void ShowTime() {
  // Calcs the time
  Elapsed = millis() - Elapsed;
}
//String Serial2in = "";
void GetSerial2Data() {
  uint8_t count_inbyte = 0;
  inputString = "";
  while (Serial2.available() > 0) {
    int8_t inbyte = Serial2.read();
    if (page > DEBUG_INFO) {
      settDataIn[count_inbyte] = inbyte;
      count_inbyte++;
    }
    char inChar = (char)inbyte;
    if (inChar) {
      inputString += inChar;
      if (inChar == '!') {
        stringComplete = true;
        break;
      }
    }
    delayMicroseconds(1750);
  }
  Serial2in = inputString;
  if (stringComplete) {
    String TempStr = inputString.substring(0, 2);
    if (TempStr == shuttnumst) {
      TempStr = inputString.substring(2, 4);
      uint8_t i = 0;
      uint8_t k = 0;
      uint8_t l = 0;
      if (TempStr == "t1") {  // get fifo, charge, status
        countcharge = 3;
        TempStr = inputString.substring(4, 5);
        int TempInt = TempStr.toInt();
        if (TempInt == 0 || TempInt == 1) fifolifo_mode = TempInt;
        for (i = 0; i <= 1; i++) {
          TempStr = inputString.substring(6 + i, 7 + i);
          if (TempStr == ":") {
            break;
          }
        }
        TempStr = inputString.substring(5, 6 + i);
        TempInt = TempStr.toInt();
        if (TempInt >= 0 && TempInt <= 50) {
          if (TempInt > 30) {
            showarn = true;
            TempInt -= 30;
          } else showarn = false;
          if (TempInt == 10 && shuttleStatus == 13) quant = 0;
          shuttleStatus = TempInt;
          if (shuttleStatus == 1) manualMode = 1;
          else manualMode = 0;
          if (shuttleStatus == 5) evacuatstatus = 1;
          else evacuatstatus = 0;
        } else shuttleStatus = 0;
        for (k = 0; k <= 2; k++) {
          TempStr = inputString.substring(8 + i + k, 9 + i + k);
          if (TempStr == ":") {
            break;
          }
        }
        TempStr = inputString.substring(7 + i, 8 + i + k);
        TempInt = TempStr.toInt();
        if (TempInt >= 0 && TempInt <= 101) {
          shuttleBattery = TempInt;
        } else {
          shuttleBattery = -1;
          newMpr = 0;
          quant = 0;
          newshuttleLength = 0;
          newspeedset = 0;
          warncode = 0;
          newlowbatt = 100;
          newreverse = 2;
          waittime = 0;
          newwaittime = 0;
          mproffset = 0;
          newmproffset = 120;
          chnloffset = 0;
          newchnloffset = 120;
        }
        TempStr = inputString.substring(9 + i + k);
        TempInt = TempStr.toInt();
        if (TempInt >= 0 && TempInt <= 99 && millis() - uctimer > 1000 && shuttleStatus != 13) {
          palletconf = TempInt;
          palletconfst = String(palletconf);
        }
        /*if (page == PACKING_CONTROL) {
          page = MAIN;
          cursorPos = 1;
        }*/

      } else if (TempStr == "t3") {  // get pallet conf получите конфигурацию поддона
        countcharge = 3;
        TempStr = inputString.substring(4);
        int TempInt = TempStr.toInt();
        if (TempInt >= 0 && TempInt <= 100 && millis() - uctimer > 1000 && shuttleStatus != 13) {
          palletconf = TempInt;
          palletconfst = String(palletconf);
        }
      } else if (TempStr == "t4") {  // get max speed, fifolifo, mpr / получите максимальную скорость, fifo lifo, mpr
        countcharge = 3;
        
        uint8_t i = 0;
        TempStr = inputString.substring(4, 5);
        fifolifo = TempStr.toInt();
        if (newreverse != 2 && newreverse != fifolifo) {fifolifo = newreverse; if (fifolifo) cmdSend(CMD_REVERSE_OFF); else cmdSend(CMD_REVERSE_ON);}
        else if (newreverse == 2) newreverse = fifolifo;
        for (i = 0; i <= 1; i++) {
          TempStr = inputString.substring(7 + i, 8 + i);
          if (TempStr == ":") {
            break;
          }
        }
        TempStr = inputString.substring(5, 7 + i);
        int TempInt = TempStr.toInt();
        if (TempInt >= 0 && TempInt <= 100) {
          speedset = TempInt;
          if (newspeedset != 0 && newspeedset != speedset) {speedset = newspeedset; cmdSend(CMD_SET_SPEED);}
        }
        for (k = 0; k <= 2; k++) {
          TempStr = inputString.substring(8 + i + k, 9 + i + k);
          if (TempStr == ":") {
            break;
          }
        }
        TempStr = inputString.substring(8 + i, 9 + i + k);
        TempInt = TempStr.toInt();
        if (TempInt >= 0 && TempInt <= 500) {
          mpr = TempInt;
          if (newMpr != 0 && newMpr != mpr) {mpr = newMpr; cmdSend(CMD_INTER_PALL_DISTANCE);}
        }
        int p = 0;
        for (p = 0; p <= 1; p++) {
          TempStr = inputString.substring(10 + i + k + p, 11 + i + k + p);
          if (TempStr == ":") {
            break;
          }
        }
        TempStr = inputString.substring(9 + i + k, 10 + i + k + p);
        TempInt = TempStr.toInt();
        if (TempInt >= 0 && TempInt <= 50) {
          lowbatt = TempInt;
          if (newlowbatt != 100 && newlowbatt != lowbatt) {lowbatt = newlowbatt; cmdSend(CMD_BATTERY_PROTECTION);}
        }
        TempStr = inputString.substring(11 + i + k + p);
        TempInt = TempStr.toInt();
        if (TempInt == 800 || TempInt == 1000 || TempInt == 1200) {
          shuttleLength = TempInt;
          if (newshuttleLength != 0 && newshuttleLength != shuttleLength) {shuttleLength = newshuttleLength; cmdSend(CMD_SET_LENGTH);}
        }
      } else if (TempStr == "rc") {  // get error получить ошибку
        countcharge = 3;
        uint8_t i = 0;
        TempStr = inputString.substring(4, 5);
        for (i = 0; i <= 4; i++) {
          TempStr = inputString.substring(6 + i, 7 + i);
          if (TempStr == ":") {
            break;
          }
        }
        TempStr = inputString.substring(5, 6 + i);
        uint16_t TempInt = TempStr.toInt();
        if (TempInt >= 0) {
          errorcode = TempInt;
        }
        TempStr = inputString.substring(7 + i);
        TempInt = TempStr.toInt();
      } else if (TempStr == "wc") {
        TempStr = inputString.substring(4, 7);
        uint8_t wrncd = TempStr.toInt();
        if (wrncd != warncode && millis() - warntimer > 2000) {warntimer = millis(); warncode = wrncd;}
      } else if (TempStr == "t6") {  //get test получить тест
        countcharge = 3;
        uint8_t i = 0;
        uint8_t k = 0;
        TempStr = inputString.substring(4, 5);
        testnum = TempStr.toInt();
        for (i = 0; i <= 1; i++) {
          TempStr = inputString.substring(6 + i, 7 + i);
          if (TempStr == ":") {
            break;
          }
        }
        TempStr = inputString.substring(5, 6 + i);
        int TempInt = TempStr.toInt();
        if (TempInt >= 0 && TempInt <= 99) {
          testtimer1 = TempInt;
        }
        for (k = 0; k <= 1; k++) {
          TempStr = inputString.substring(8 + i + k, 9 + i + k);
          if (TempStr == ":") {
            break;
          }
        }
        TempStr = inputString.substring(7 + i, 8 + i + k);
        TempInt = TempStr.toInt();
        if (TempInt >= 0 && TempInt <= 99) {
          testtimer2 = TempInt;
        }
        TempStr = inputString.substring(9 + i + k);
        TempInt = TempStr.toInt();
        if (TempInt >= 0) {
          testrepeat = TempInt;
        }
      } else if (TempStr == "FL") {  //get fifo away, fifo/lifo mode уберите fifo, режим fifo/lifo
        countcharge = 3;
        TempStr = inputString.substring(4, 5);
        int TempInt = TempStr.toInt();
        if (TempInt == 0 || TempInt == 1) shuttbackaway = TempInt;
        TempStr = inputString.substring(5, 6);
        TempInt = TempStr.toInt();
        if (TempInt == 0 || TempInt == 1) fifolifo_mode = TempInt;
      } else if (TempStr == "pq") {  //get downloads quant pallet получите поддон количества загрузок
        countcharge = 3;
        TempStr = inputString.substring(4);
        int TempInt = TempStr.toInt();
        if (TempInt > 0 && TempInt < 100) {
          quant = TempInt;
          shuttleStatus = 13;
        } else quant = 0;
        Serial.println("Quant 2 = " + String(quant) + " string: " + TempStr + " fullstr: " + inputString);
      } else if (TempStr == "ml") {  //get minlevel получить минимальный уровень
        countcharge = 3;
        TempStr = inputString.substring(4);
        int TempInt = TempStr.toInt();
        if (TempInt >= 0 && TempInt <= 30) {
          minlevel = TempInt;
        } else minlevel = 31;
      } else if (TempStr == "st") {  //get statistic odometr получите статистику одометра
        countcharge = 0;
        TempStr = inputString.substring(4, 5);
        int TempStat = TempStr.toInt();
        TempStr = inputString.substring(6);
        int TempInt = TempStr.toInt();
        if (TempStat >= 0 && TempStat < 10 && TempInt >= 0) {
          statistic[TempStat] = TempInt;
          getchargetime = millis();
          if (TempInt >= 0) {
            //countcharge = 0;
            odometr_pos = TempStat + 1;
            if (odometr_pos < 10) {
              //cmdStatistic();
              cyclegetcharge = 0;
              //countcharge = 0;
            }
          }
        }
      } else if (TempStr == "iu") {  // калибровка
        TempStr = inputString.substring(2, 8);
        int Tempoffs = TempStr.toInt();
        if (Tempoffs > 0) {
          calibret = Tempoffs;
          calibret = "Да";
        } else calibret = "Нет";
      } else if (TempStr == "yt") {
        countcharge = 3;
        TempStr = inputString.substring(4, 8);
        int TempInt = TempStr.toInt();
        if (TempInt >= 0 && TempInt <= 1500) {
          sensor_channel_f = TempInt;
        }

        TempStr = inputString.substring(9, 13);
        int Tempint = TempStr.toInt();
        if (Tempint >= 0 && Tempint <= 1500) {
          sensor_channel_r = Tempint;
        }

        TempStr = inputString.substring(14, 18);
        int Temp_sens_pl_f = TempStr.toInt();
        if (Temp_sens_pl_f >= 0 && Temp_sens_pl_f <= 1500) {
          sensor_pallete_F = Temp_sens_pl_f;
        }

        TempStr = inputString.substring(19, 23);
        int Temp_sens_pl_r = TempStr.toInt();
        if (Temp_sens_pl_r >= 0 && Temp_sens_pl_r <= 1500) {
          sensor_pallete_R = Temp_sens_pl_r;
        }

        TempStr = inputString.substring(24, 28);
        int Temp_enc = TempStr.toInt();
        if (Temp_enc >= 0 && Temp_enc <= 4096) {
          enc_mm = Temp_enc;
        }

        TempStr = inputString.substring(29, 30);
        int tempInt = TempStr.toInt();
        if (tempInt >= 0) {
          DATCHIK_F1 = tempInt;
        }

        TempStr = inputString.substring(31, 32);
        int tempint = TempStr.toInt();
        if (tempint >= 0) {
          DATCHIK_F2 = tempint;
        }

        TempStr = inputString.substring(33, 34);
        int Temp_DATCH_r1 = TempStr.toInt();
        if (Temp_DATCH_r1 >= 0) {
          DATCHIK_R1 = Temp_DATCH_r1;
        }

        TempStr = inputString.substring(35, 36);
        int Temp_DATCH_r2 = TempStr.toInt();
        if (Temp_DATCH_r2 >= 0) {
          DATCHIK_R2 = Temp_DATCH_r2;
        }
      } else if (TempStr == "lg") {
        TempStr = inputString.substring(4, 5);
        logWrite = TempStr.toInt();
      } else if (TempStr == "wt") {
        TempStr = inputString.substring(4, 6);
        waittime = TempStr.toInt();
        if (newwaittime && newwaittime != waittime) {waittime = newwaittime; cmdSend(CMD_WAIT_TIME);}
      } else if (TempStr == "wo") {
        TempStr = inputString.substring(4, 7);
        mproffset = TempStr.toInt() - 100;
        if (newmproffset != 120 && newmproffset != mproffset) {mproffset = newmproffset; cmdSend(CMD_MPR_OFFSET);}
        TempStr = inputString.substring(8, 11);
        chnloffset = TempStr.toInt() - 100;
        if (newchnloffset != 120 && newchnloffset != chnloffset) {chnloffset = newchnloffset; cmdSend(CMD_CHNL_OFFSET);}
      }
    }
    UpdateParam = true;
    //}
    displayUpdate = true;
  }
  stringComplete = false;  
}

String GetSerial2Ans_() {
  int8_t inByte = 0;
  String inStr = "";
  if (Serial2.available()) // Получаем команду
  {
    inByte = Serial2.read();
    char inChar = (char) inByte;
    inStr += inChar;
    delayMicroseconds(1750);
    inByte = Serial2.read();
    inChar = (char) inByte;
    inStr += inChar;
    while(inStr != shuttnumst && Serial2.available())
    {
      inStr = inStr.substring(1,2);
      delayMicroseconds(1750);
      inByte = Serial2.read();
      inChar = (char) inByte;
      inStr += inChar;
    }
    if (inStr == shuttnumst) 
    {
      int cnt = millis();
      while(inStr.length() <= 7 && millis() - cnt < 50)
      {
        delayMicroseconds(100);
        if (Serial2.available())
        {
          inByte = Serial2.read();
          inChar = (char) inByte;
          inStr += inChar;
        }
      }
      if (inStr.length() == 8) return inStr;
    }
  }
  return inStr;
}
String GetSerial2Ans() {
  int8_t inByte = 0;
  String inStr = "";
  while (Serial2.available()) // Получаем команду
  {
    inByte = Serial2.read();
    char inChar = (char) inByte;
    inStr += inChar;
    delayMicroseconds(1750);
  }
  return inStr;
}

void SetSleep() {
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
  digitalWrite(rfout0, LOW);
  gpio_hold_en((gpio_num_t)rfout0);

  //gpio_deep_sleep_hold_en();
  delay(100);
  esp_deep_sleep_start();
}
void BatteryLevel(uint8_t percent) {
  uint8_t xsize = 0;
  if (percent < prevpercent || percent > prevpercent + 5) {
    prevpercent = percent;
  }
  if (prevpercent > 95) xsize = 14;
  else if (prevpercent > 75) xsize = 11;
  else if (prevpercent > 50) xsize = 8;
  else if (prevpercent > 25) xsize = 5;
  else if (prevpercent > 7) xsize = 2;
  else xsize = 0;
  u8g2.drawFrame(107, 1, 18, 10);
  u8g2.drawBox(125, 4, 2, 4);
  if (xsize) u8g2.drawBox(109, 3, xsize, 6);
  //u8g2.setCursor(100, 25);
  //u8g2.print(String(percent));
  //u8g2.print(String(analogRead(Battery_Pin)));
  //if (digitalRead(Charge_Pin)) u8g2.print(" ON");
  //else u8g2.print(" OFF");
}
void MenuOut() {
  if (cursorPos < 6)
    u8g2.drawBox(0, 6 + (cursorPos - 1) * 11, 128, 11);
  else u8g2.drawBox(0, 50, 128, 11);
  for (uint8_t i = 1; i < 6; i++) {
    u8g2.setCursor(0, 5 + i * 11);
    if (cursorPos == i || (cursorPos > 5 && i == 5)) u8g2.setDrawColor(0);
    else u8g2.setDrawColor(1);
    if (cursorPos < 6) u8g2.print(strmenu[i - 1]);
    else u8g2.print(strmenu[cursorPos + i - 6]);
  }
}
#pragma endregion