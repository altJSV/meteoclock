/*
  Скетч к проекту "Домашняя метеостанция"
  Страница проекта (схемы, описания): https://alexgyver.ru/meteoclock/
  Исходники на GitHub: https://github.com/AlexGyver/MeteoClock
  Нравится, как написан и закомментирован код? Поддержи автора! https://alexgyver.ru/support_alex/
  Автор: AlexGyver Technologies, 2018
  http://AlexGyver.ru/
  Доработал Роман Новиков (с)НР
  Доработка Web интерфейса, правка багов и адаптация под Wemos D1 Mini Яровой Сергей
  Обновления на https://projectalt.ru
*/

/*
  Время и дата устанавливаются атвоматически при загрузке прошивки (такие как на компьютере)
  График всех величин за час и за сутки (усреднённые за каждый час)
  В модуле реального времени стоит батарейка, которая продолжает отсчёт времени после выключения/сброса питания
  Как настроить время на часах. У нас есть возможность автоматически установить время на время загрузки прошивки, поэтому:
	- Ставим настройку RESET_CLOCK на 1
  - Прошиваемся
  - Сразу ставим RESET_CLOCK 0
  - И прошиваемся ещё раз
  - Всё
*/

/* Версия 1.5
  - Добавлено управление яркостью
  - Яркость дисплея и светодиода СО2 меняется на максимальную и минимальную в зависимости от сигнала с фоторезистора
  Подключите датчик (фоторезистор) по схеме. Теперь на экране отладки справа на второй строчке появится величина сигнала
  с фоторезистора.
*/

/* Версия 2020
  - Портировал на ESP8266 12E                                       
  - Синхронизация времени через NTP
  - Вэб страница часов с параметрами и страницей настроек
  - Параметры сохраняются в файловую систему LittleFS
  - Обновление прошивки "по воздуху" (через вэб страницу часов, но первый раз придется прошить через COM порт)
  - Подключение к MQTT брокеру и отправка показаний температуры, давления, влажности, содержания CO2
  - После включения подымается WiFi точка 192.168.4.1 (адрес будет на LCD, CLOCK-SSID). 
    Нужно зайти на страницу и прописать параметри подключения к WiFi, часовой пояс, данные для MQTT брокера
    В дальнейшем часы будут получать адрес от вашей сети
*/

// ------------------------- НАСТРОЙКИ --------------------
#define RESET_CLOCK 0      // сброс часов на время загрузки прошивки (для модуля с несъёмной батарейкой). Не забудь поставить 0 и прошить ещё раз!
#define SENS_TIME 300000   // время обновления показаний сенсоров на экране, миллисекунд
#define NTPSYNCTIME 600000 //период синхронизации с NTP сервером
#define LED_MODE 0        // тип RGB светодиода: 0 - главный катод, 1 - главный анод
#define SEALEVELPRESSURE_HPA (1013.25) // Коэффициент для расчета высоты над уровнем моря

#ifndef STASSID
#define STASSID "CLOCK-SSID"
#define STAPSK  "0987654321"
#endif
String hostName ="WiFiClock";
char* host; //The DNS hostname
int otaCount=300; //timeout in sec for OTA mode
int otaFlag=0;
int otaFlag_=0;
#include <Ticker.h>
Ticker otaTickLoop;

// управление яркостью
byte LED_BRIGHT = 10;         // при отсутствии сохранения в EEPROM: яркость светодиода СО2 (0 - 10) (коэффициент настраиваемой яркости индикатора по умолчанию, если нет сохранения и не автоматическая регулировка (с)НР)
byte LCD_BRIGHT = 10;         // при отсутствии сохранения в EEPROM: яркость экрана (0 - 10) (коэффициент настраиваемой яркости экрана по умолчанию, если нет сохранения и не автоматическая регулировка (с)НР)
byte powerStatus = 0;         // индикатор вида питания: 255 - не показывать, остальное автоматически (0 - от сети, 1-5 уровень заряда батареи) (с)НР
#define BRIGHT_CONTROL 1      // 0/1 - запретить/разрешить управление яркостью (при отключении яркость всегда будет макс.)
#define BRIGHT_THRESHOLD 350  // величина сигнала, ниже которой яркость переключится на минимум (0-1023)
#define LED_BRIGHT_MAX 255    // макс яркость светодиода СО2 (0 - 255)
#define LED_BRIGHT_MIN 10     // мин яркость светодиода СО2 (0 - 255)
#define LCD_BRIGHT_MAX 255    // макс яркость подсветки дисплея (0 - 255)
#define LCD_BRIGHT_MIN 10     // мин яркость подсветки дисплея (0 - 255)

#define DISP_MODE 1       // в правом верхнем углу отображать: 0 - год, 1 - день недели и секунды
#define WEEK_LANG 1       // язык дня недели: 0 - английский, 1 - русский
#define PRESSURE 0        // 0 - график давления, 1 - график прогноза дождя (вместо давления). Не забудь поправить пределы графика
#define CO2_SENSOR 0      // включить или выключить поддержку/вывод с датчика СО2 (1 вкл, 0 выкл)
#define DISPLAY_TYPE 1    // тип дисплея: 1 - 2004 (большой), 0 - 1602 (маленький)
#define DISPLAY_ADDR 0x27 // адрес платы дисплея: 0x27 или 0x3f. Если дисплей не работает - смени адрес! На самом дисплее адрес не указан

// пределы для индикатора (с)НР
#define normCO2 800       // и выше - желтый
#define maxCO2 1200       // и выше - красный
#define blinkLEDCO2 1500  // Значение СО2, при превышении которого будет мигать индикатор

#define minTemp 21        // и ниже для синего индикатора
#define normTemp 26       // и выше - желтый
#define maxTemp 28        // и выше - красный
#define blinkLEDTemp 35   // Значение температуры, при превышении которой будет мигать индикатор

#define maxHum 90         // и выше - синий
#define normHum 30        // и ниже - желтый
#define minHum 20         // и ниже - красный
#define blinkLEDHum 15    // Значение влажности, при показаниях ниже порога которого будет мигать индикатор

#define normPress 733     // и ниже - желтый
#define minPress 728      // и ниже - красный   // может, переделать на синий?

#define minRain -50       // и ниже - красный   
#define normRain -20      // и ниже - желтый
#define maxRain 50        // и выше - синий

// пределы отображения для графиков
#define TEMP_MIN 15
#define TEMP_MAX 35
#define HUM_MIN 0
#define HUM_MAX 100
#define PRESS_MIN 720
#define PRESS_MAX 760
#define CO2_MIN 400
#define CO2_MAX 2000
#define ALT_MIN 0
#define ALT_MAX 1000


// пины
#define BACKLIGHT 13

#define PHOTO A0 // пин фоторезистора 

#define BTN_PIN 15  // пин кнопки притянутый до земли резистором 10 кОм (сенсорный ТТР223 можно без резистора)

#define MHZ_RX 0
#define MHZ_TX 2

#define LED_COM 10
#define LED_R 12
#define LED_G 14
#define LED_B 16
#define DS3231_I2C_ADDRESS 0x68

byte LEDType = 0;         //  при отсутствии сохранения в EEPROM: привязка индикатора к датчикам: 0 - СО2, 1 - Влажность, 2 - Температура, 3 - Осадки

// библиотеки
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <GyverNTP.h>
//#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>
#include "RTClib.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "FS.h"
#include <LittleFS.h>
extern "C" {
  #include "user_interface.h" //Needed for the reset command
}

#if (CO2_SENSOR == 1)
  #include <MHZ19_uart.h>
  MHZ19_uart mhz19;
#endif

#include "GyverButton.h"

int MAX_ONDATA = 1 + 2 + 4 + 8 + 16 + 32 + 64 + 128 + 256 + 512 + 1024 + 2048; // при отсутствии сохранения в EEPROM: максимальные показания графиков исходя из накопленных фактических (но в пределах лимитов) данных вместо указанных пределов, 0 - использовать фиксированные пределы (с)НР
int VIS_ONDATA = 1 + 2 + 4 + 8 + 16 + 32 + 64 + 128 + 256 + 512 + 1024 + 2048; // при отсутствии сохранения в EEPROM: отображение показания графиков, 0 - Не отображать (с)НР

/* 1 - для графика СО2 часового, 2 - для графика СО2 суточного (с)НР
   4 - для графика влажности часовой, 8 - для графика влажности суточной (с)НР
   16 - для графика температуры часовой, 32 - для графика температуры суточной (с)НР
   64 - для прогноза дождя часового, 128 - для прогноза дождя суточного (с)НР
   256 - для графика давления часового, 512 - для графика давления суточного (с)НР
   1024 - для графика высоты часового, 2048 - для графика высоты суточного (с)НР
   для выборочных графиков значения нужно сложить (с)НР
   например: для изменения пределов у графиков суточной температуры и суточного СО2 складываем 2 + 32 и устанавливаем значение 34 (можно ставить сумму) (с)НР
*/
const char* clock_ssid = STASSID;
const char* clock_pass = STAPSK;

// NTP Servers:
//static const char ntpServerName[] = "us.pool.ntp.org";
static const char ntpServerName[] = "time.nist.gov";
//static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";
//static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";

String ssid="Keenetic-8995";       //  your network SSID (name)
String pass="S1e9r8g5ey";       // your network password
int8_t TIMEZONE = 3; // Asia/Sakhalin Time
String mqtt_ip;    // MQTT ip address
unsigned int ip_addr[]={0,0,0,0};
String mqtt_port;  // MQTT port
String mqtt_auth;  // MQTT user name
String mqtt_pass;  // MQTT user password
String mqtt_CO2;   // MQTT топик датчика CO2
String mqtt_Hum;   // MQTT топик датчика влажности
String mqtt_Temp;  // MQTT топик датчика температуры
String mqtt_Press; // MQTT топик датчика давления

int photocellReading; //+AB+ яркость с фоторезистора
int LCD_bright_constrain; //+AB+ яркость LCD после приведения диапазонов
int LCD_brightness; //+AB+ яркость LCD текущая
byte decToBcd(byte val){  return ( (val / 10 * 16) + (val % 10) );}
byte bcdToDec(byte val){  return ( (val / 16 * 10) + (val % 16) );}
String tmp_arg_st;
int tmp_arg_int;
WiFiClient espClient;
IPAddress mqtt_server(0, 0, 0, 0);
PubSubClient client(espClient, mqtt_server);

WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
//time_t getNtpTime();
GyverNTP ntp(TIMEZONE);
void sendNTPpacket(IPAddress &address);
ESP8266WebServer server(80);

#if (DISPLAY_TYPE == 1)
LiquidCrystal_I2C lcd(DISPLAY_ADDR, 20, 4);
#else
LiquidCrystal_I2C lcd(DISPLAY_ADDR, 16, 2);
#endif

RTC_DS3231 rtc;

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme;

unsigned long sensorsTimer = SENS_TIME;
unsigned long drawSensorsTimer = SENS_TIME;
unsigned long clockTimer = 500;
unsigned long ntpTimer = NTPSYNCTIME;

#if (DISPLAY_TYPE == 1)           // для дисплея 2004 график "длиннее", поэтому интервалы времени на сегмент короче (с)НР
unsigned long hourPlotTimer = ((long)4 * 60 * 1000);        // 4 минуты
unsigned long dayPlotTimer = ((long)1.6 * 60 * 60 * 1000);  // 1.6 часа
#else
unsigned long hourPlotTimer = ((long)5 * 60 * 1000);        // 5 минуты
unsigned long dayPlotTimer = ((long)2 * 60 * 60 * 1000);    // 2 часа
#endif

unsigned long predictTimer = ((long)10 * 60 * 1000);        // 10 минут
unsigned long plotTimer = hourPlotTimer;
unsigned long brightTimer = (2000);

unsigned long RTCsyncTimer = ((long)1 * 10 * 60 * 1000);    // 10 минут
unsigned long RTCsyncTimerD = 0;

unsigned long WEBsyncTimer = ((long)3 * 1000);    // 3 сек
unsigned long WEBsyncTimerD = 0;

unsigned long MQTTsyncTimer = ((long)15 * 1000);    // 15 сек
unsigned long MQTTsyncTimerD = 0;

unsigned long sensorsTimerD = 0;
unsigned long drawSensorsTimerD = 0;
unsigned long clockTimerD = 0;
unsigned long ntpTimerD = 0;
unsigned long hourPlotTimerD = 0;
unsigned long dayPlotTimerD = 0;
unsigned long plotTimerD = 0;
unsigned long predictTimerD = 0;
unsigned long brightTimerD = 0;

GButton button(BTN_PIN, LOW_PULL, NORM_OPEN);

int8_t hrs, mins, secs;
byte mode = 0;
/*
  0 часы и данные
  1 график углекислого за час
  2 график углекислого за сутки
  3 график влажности за час
  4 график влажности за сутки
  5 график температуры за час
  6 график температуры за сутки
  7 график дождя/давления за час
  8 график дождя/давления за сутки
  9 график высоты за час
  10 график высоты за сутки
*/

byte podMode = 1; // подрежим меню(с)НР
byte mode0scr = 0;
/* (с)НР
  0 - Крупно время
  1 - Крупно содержание СО2
  2 - Крупно температура
  3 - Крупно давление
  4 - Крупно влажность
  5 - Крупно высота
*/
boolean bigDig = false;   // true - цифры на главном экране на все 4 строки (для LCD 2004) (с)НР

// переменные для вывода
float dispTemp;
byte dispHum;
int dispPres;
int dispCO2 = -1;
int dispRain;
float dispAlt;  //int
char myIP[] = "000.000.000.000";

// массивы графиков
int tempHour[15], tempDay[15];
//#define tempK 40                //поправочный поэффициент, чтобы показания влезли в байт
int humHour[15], humDay[15];
int pressHour[15], pressDay[15];
//#define pressK -600             //поправочный поэффициент, чтобы показания влезли в байт
int rainHour[15], rainDay[15];
//#define rainK 100               //поправочный поэффициент, чтобы показания влезли в байт
int co2Hour[15], co2Day[15];
int altHour[15], altDay[15];      // высота
int delta;
uint32_t pressure_array[6];
uint32_t sumX, sumY, sumX2, sumXY;
float a, b;
//byte time_array[6];

/*
  Характеристики датчика BME:
  Температура: от-40 до + 85 °C
  Влажность: 0-100%
  Давление: 300-1100 hPa (225-825 ммРтСт)
  Разрешение:
  Температура: 0,01 °C
  Влажность: 0.008%
  Давление: 0,18 Pa
  Точность:
  Температура: +-1 °C
  Влажность: +-3%
  Давление: +-1 Па
*/

// символы
// график
byte rowS[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b10001,  0b01010,  0b00100,  0b00000};   // стрелка вниз (с)НР
byte row7[8] = {0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row6[8] = {0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};
byte row5[8] = {0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};   // в т.ч. для четырехстрочных цифр 2, 3, 4, 5, 6, 8, 9, 0 (с)НР
byte row4[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111,  0b11111};
byte row3[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111};   // в т.ч. для двустрочной цифры 0, для четырехстрочных цифр 2, 3, 4, 5, 6, 8, 9 (с)НР
byte row2[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111};   // в т.ч. для двустрочной цифры 4 (с)НР
byte row1[8] = {0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111};

// цифры //  (с)НР
uint8_t UB[8] = {0b11111,  0b11111,  0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000};   // для двустрочных 7, 0   // для четырехстрочных 2, 3, 4, 5, 6, 8, 9
uint8_t UMB[8] = {0b11111,  0b11111,  0b11111,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111};  // для двустрочных 2, 3, 5, 6, 8, 9
uint8_t LMB[8] = {0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b11111,  0b11111,  0b11111};  // для двустрочных 2, 3, 5, 6, 8, 9
uint8_t LM2[8] = {0b11111,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000,  0b00000};  // для двустрочной 4
uint8_t UT[8] = {0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b00000,  0b00000,  0b00000};   // для четырехстрочных 2, 3, 4, 5, 6, 7, 8, 9, 0

uint8_t KU[8] = {0b00000,  0b00000,  0b00000,  0b00001,  0b00010,  0b00100,  0b01000,  0b10000};   // для верхней части %
uint8_t KD[8] = {0b00001,  0b00010,  0b00100,  0b01000,  0b10000,  0b00000,  0b00000,  0b00000};   // для нижней части %

// русские буквы (с)НР
uint8_t PP[8] = {0b11111,  0b10001,  0b10001,  0b10001,  0b10001,  0b10001,  0b10001,  0b00000};  // П
uint8_t BB[8] = {0b11111,  0b10000,  0b10000,  0b11111,  0b10001,  0b10001,  0b11111,  0b00000};  // Б
uint8_t CH[8] = {0b10001,  0b10001,  0b10001,  0b01111,  0b00001,  0b00001,  0b00001,  0b00000};  // Ч
uint8_t II[8] = {0b10001,  0b10001,  0b10011,  0b10101,  0b11001,  0b10001,  0b10001,  0b00000};  // И
uint8_t BM[8] = {0b10000,  0b10000,  0b10000,  0b11110,  0b10001,  0b10001,  0b11110,  0b00000};  // Ь
uint8_t IY[8] = {0b01100,  0b00001,  0b10011,  0b10101,  0b11001,  0b10001,  0b10001,  0b00000};  // Й
uint8_t DD[8] = {0b01110,  0b01010,  0b01010,  0b01010,  0b01010,  0b01010,  0b11111,  0b10001};  // Д
uint8_t AA[8] = {0b11100,  0b00010,  0b00001,  0b00111,  0b00001,  0b00010,  0b11100,  0b00000};  // Э
uint8_t IA[8] = {0b01111,  0b10001,  0b10001,  0b01111,  0b00101,  0b01001,  0b10001,  0b00000};  // Я
uint8_t YY[8] = {0b10001,  0b10001,  0b10001,  0b11101,  0b10011,  0b10011,  0b11101,  0b00000};  // Ы
uint8_t GG[8] = {0b11110,  0b10000,  0b10000,  0b10000,  0b10000,  0b10000,  0b10000,  0b00000};  // Г
uint8_t FF[8] = {0b00100,  0b01110,  0b10101,  0b10101,  0b10101,  0b01110,  0b00100,  0b00000};  // Ф
uint8_t LL[8] = {0b01111,  0b01001,  0b01001,  0b01001,  0b01001,  0b01001,  0b10001,  0b00000};  // Л
uint8_t ZZ[8] = {0b10101,  0b10101,  0b10101,  0b01110,  0b10101,  0b10101,  0b10101,  0b00000};  // Ж

// индикатор питания (с)НР
// сеть
uint8_t AC[8] = {0b01010,  0b01010,  0b11111,  0b11111,  0b01110,  0b00100,  0b00100,  0b00011};
// батарея
uint8_t DC[8] = {0b01110,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111,  0b11111};    // уровень батареи - изменяется в коде скетча (с)НР

void digSeg(byte x, byte y, byte z1, byte z2, byte z3, byte z4, byte z5, byte z6) {   // отображение двух строк по три символа с указанием кодов символов (с)НР
  lcd.setCursor(x, y);
  lcd.write(z1); lcd.write(z2); lcd.write(z3);
  if (x <= 11) lcd.print(" ");
  lcd.setCursor(x, y + 1);
  lcd.write(z4); lcd.write(z5); lcd.write(z6);
  if (x <= 11) lcd.print(" ");
}

void drawDig(byte dig, byte x, byte y) {        // рисуем цифры (с)НР ---------------------------------------
  if (bigDig && DISPLAY_TYPE == 1) {
    switch (dig) {            // четырехстрочные цифры (с)НР
      case 0:
        digSeg(x, y, 255, 0, 255, 255, 32, 255);
        digSeg(x, y + 2, 255, 32, 255, 255, 3, 255);
        break;
      case 1:
        digSeg(x, y, 32, 255, 32, 32, 255, 32);
        digSeg(x, y + 2, 32, 255, 32, 32, 255, 32);
        break;
      case 2:
        digSeg(x, y, 0, 0, 255, 1, 1, 255);
        digSeg(x, y + 2, 255, 2, 2, 255, 3, 3);
        break;
      case 3:
        digSeg(x, y, 0, 0, 255, 1, 1, 255);
        digSeg(x, y + 2, 2, 2, 255, 3, 3, 255);
        break;
      case 4:
        digSeg(x, y, 255, 32, 255, 255, 1, 255);
        digSeg(x, y + 2, 2, 2, 255, 32, 32, 255);
        break;
      case 5:
        digSeg(x, y, 255, 0, 0, 255, 1, 1);
        digSeg(x, y + 2, 2, 2, 255, 3, 3, 255);
        break;
      case 6:
        digSeg(x, y, 255, 0, 0, 255, 1, 1);
        digSeg(x, y + 2, 255, 2, 255, 255, 3, 255);
        break;
      case 7:
        digSeg(x, y, 0, 0, 255, 32, 32, 255);
        digSeg(x, y + 2, 32, 255, 32, 32, 255, 32);
        break;
      case 8:
        digSeg(x, y, 255, 0, 255, 255, 1, 255);
        digSeg(x, y + 2, 255, 2, 255, 255, 3, 255);
        break;
      case 9:
        digSeg(x, y, 255, 0, 255, 255, 1, 255);
        digSeg(x, y + 2, 2, 2, 255, 3, 3, 255);
        break;
      case 10:
        digSeg(x, y, 32, 32, 32, 32, 32, 32);
        digSeg(x, y + 2, 32, 32, 32, 32, 32, 32);
        break;
    }
  }
  else {
    switch (dig) {            // двухстрочные цифры
      case 0:
        digSeg(x, y, 255, 1, 255, 255, 2, 255);
        break;
      case 1:
        digSeg(x, y, 32, 255, 32, 32, 255, 32);
        break;
      case 2:
        digSeg(x, y, 3, 3, 255, 255, 4, 4);
        break;
      case 3:
        digSeg(x, y, 3, 3, 255, 4, 4, 255);
        break;
      case 4:
        digSeg(x, y, 255, 0, 255, 5, 5, 255);
        break;
      case 5:
        digSeg(x, y, 255, 3, 3, 4, 4, 255);
        break;
      case 6:
        digSeg(x, y, 255, 3, 3, 255, 4, 255);
        break;
      case 7:
        digSeg(x, y, 1, 1, 255, 32, 255, 32);
        break;
      case 8:
        digSeg(x, y, 255, 3, 255, 255, 4, 255);
        break;
      case 9:
        digSeg(x, y, 255, 3, 255, 4, 4, 255);
        break;
      case 10:
        digSeg(x, y, 32, 32, 32, 32, 32, 32);
        break;
    }
  }
}

void drawPPM(int dispCO2, byte x, byte y) {     // Уровень СО2 крупно на главном экране (с)НР ----------------------------
  if (dispCO2 / 1000 == 0) drawDig(10, x, y);
  else drawDig(dispCO2 / 1000, x, y);
  drawDig((dispCO2 % 1000) / 100, x + 4, y);
  drawDig((dispCO2 % 100) / 10, x + 8, y);
  drawDig(dispCO2 % 10 , x + 12, y);
  lcd.setCursor(15, 0);
#if (DISPLAY_TYPE == 1)
  lcd.print("ppm");
#else
  lcd.print("p");
#endif
}

void drawPres(int dispPres, byte x, byte y) {   // Давление крупно на главном экране (с)НР ----------------------------
  drawDig((dispPres % 1000) / 100, x , y);
  drawDig((dispPres % 100) / 10, x + 4, y);
  drawDig(dispPres % 10 , x + 8, y);
  lcd.setCursor(x + 11, 1);
  if (bigDig) lcd.setCursor(x + 11, 3);
  lcd.print("mm");
}

void drawAlt(float dispAlt, byte x, byte y) {   // Высота крупно на главном экране (с)НР -----------------------------
  if (dispAlt >= 1000) {
    drawDig((int(dispAlt) % 10000) / 1000, x , y);
    x += 4;
  }
  drawDig((int(dispAlt) % 1000) / 100, x , y);
  drawDig((int(dispAlt) % 100) / 10, x + 4, y);
  drawDig(int(dispAlt) % 10 , x + 8, y);
  if (dispAlt < 1000) {       // десятые доли метра, если высота ниже 1000 м. (с)НР
    //   drawDig((int(dispAlt * 10.0)) % 10 , x + 12, y);         // десятые крупными цифрами (тогда буква m наезжает на последнюю цифру)
    lcd.setCursor(x + 12, y + 1 + (bigDig && DISPLAY_TYPE) * 2);  // десятые мелкими цифрами
    lcd.print((int(dispAlt * 10.0)) % 10);
    if (bigDig && DISPLAY_TYPE == 1) lcd.setCursor(x + 11, y + 3);
    else lcd.setCursor(x + 11, y + 1);
    lcd.print(".");
    x -= 1; // сдвинуть букву m левее
  }  else {
    x -= 4;
  }
  if (bigDig && DISPLAY_TYPE == 1) lcd.setCursor(x + 14, 3);
  else lcd.setCursor(x + 14, 1);
  lcd.print("m");
}

void drawTemp(float dispTemp, byte x, byte y) { // Температура крупно на главном экране (с)НР ----------------------------
  if (dispTemp / 10 == 0) drawDig(10, x, y);
  else drawDig(dispTemp / 10, x, y);
  drawDig(int(dispTemp) % 10, x + 4, y);
  drawDig(int(dispTemp * 10.0) % 10, x + 9, y);

  if (bigDig && DISPLAY_TYPE == 1) {
    lcd.setCursor(x + 7, y + 3);
    lcd.write(1);             // десятичная точка
  }
  else {
    lcd.setCursor(x + 7, y + 1);
    lcd.write(0B10100001);    // десятичная точка
  }
  lcd.setCursor(x + 13, y);
  lcd.write(223);             // градусы
}

void drawHum(int dispHum, byte x, byte y) {   // Влажность крупно на главном экране (с)НР ----------------------------
  if (dispHum / 100 == 0) drawDig(10, x, y);
  else drawDig(dispHum / 100, x, y);
  if ((dispHum % 100) / 10 == 0) drawDig(0, x + 4, y);
  else drawDig(dispHum / 10, x + 4, y);
  drawDig(int(dispHum) % 10, x + 8, y);
  if (bigDig && DISPLAY_TYPE == 1) {
    lcd.setCursor(x + 12, y + 1);
    lcd.print("\245\4");
    lcd.setCursor(x + 12, y + 2);
    lcd.print("\5\245");
  }
  else {
    lcd.setCursor(x + 12, y + 1);
    lcd.print("%");
  }
}

void drawClock(byte hours, byte minutes, byte x, byte y) {    // рисуем время крупными цифрами -------------------------------------------
  if (hours > 23 || minutes > 59) return;
  if (hours / 10 == 0) drawDig(10, x, y);
  else drawDig(hours / 10, x, y);
  drawDig(hours % 10, x + 4, y);
  // тут должны быть точки. Отдельной функцией
  drawDig(minutes / 10, x + 8, y);
  drawDig(minutes % 10, x + 12, y);
}

#if (WEEK_LANG == 0)
static const char *dayNames[]  = {
  "Su",
  "Mo",
  "Tu",
  "We",
  "Th",
  "Fr",
  "Sa",
};
#else
static const char *dayNames[]  = {  // доработал дни недели на двухсимвольные русские (ПН, ВТ, СР....) (с)НР
  "BC",
  "\7H",
  "BT",
  "CP",
  "\7T",
  "\7T",
  "C\7",
};
#endif

void drawData() {                     // выводим дату -------------------------------------------------------------
  int Y = 0;
  DateTime now =rtc.now();
  if (DISPLAY_TYPE == 1 && mode0scr == 1) Y = 2;
  if (!bigDig) {              // если 4-х строчные цифры, то дату, день недели (и секунды) не пишем - некуда (с)НР
    lcd.setCursor(15, 0 + Y);
    if (now.day() < 10) lcd.print(0);
    lcd.print(now.day());
    lcd.print(".");
    if (now.month() < 10) lcd.print(0);
    lcd.print(now.month());

    if (DISP_MODE == 0) {
      lcd.setCursor(16, 1);
      lcd.print(now.year());
    } else {
      loadClock();              // принудительно обновляем знаки, т.к. есть жалобы на необновление знаков в днях недели (с)НР
      lcd.setCursor(18, 1);
      int dayofweek = now.dayOfTheWeek();
      lcd.print(dayNames[dayofweek]);
      // if (hrs == 0 && mins == 0 && secs <= 1) loadClock();   // Обновляем знаки, чтобы русские буквы в днях недели тоже обновились. (с)НР
    }
  }
}

void drawPlot(byte pos, byte row, byte width, byte height, int min_val, int max_val, int *plot_array, String label1, String label2, int stretch) {  // график ---------------------------------
  int max_value = -32000;
  int min_value = 32000;

  for (byte i = 0; i < 15; i++) {
    max_value = max(plot_array[i] , max_value);
    min_value = min(plot_array[i] , min_value);
  }

  // меняем пределы графиков на предельные/фактические значения, одновременно рисуем указатель пределов (стрелочки вверх-вниз) (с)НР
  lcd.setCursor(15, 0);
  if ((MAX_ONDATA & (1 << (stretch - 1))) > 0) {    // побитовое сравнение 1 - растягиваем, 0 - не растягиваем (по указанным пределам) (с)НР
    //    max_val = min(max_value, max_val);
    //    min_val = max(min_value, min_val);
    max_val = max_value;
    min_val = min_value;
#if (DISPLAY_TYPE == 1)
    lcd.write(0b01011110);
    lcd.setCursor(15, 3);
    lcd.write(0);
#endif
  }  else {
#if (DISPLAY_TYPE == 1)
    lcd.write(0);
    lcd.setCursor(15, 3);
    lcd.write(0b01011110);
#endif
  }

  if (min_val >= max_val) max_val = min_val + 1;
#if (DISPLAY_TYPE == 1)
  lcd.setCursor(15, 1); lcd.write(0b01111100);
  lcd.setCursor(15, 2); lcd.write(0b01111100);

  //Serial.println(max_val);Serial.println(min_val);  // отладка (с)НР

  lcd.setCursor(16, 0); lcd.print(max_value);
  lcd.setCursor(16, 1); lcd.print(label1); lcd.print(label2);
  lcd.setCursor(16, 2); lcd.print(plot_array[14]);
  lcd.setCursor(16, 3); lcd.print(min_value);
#else
  lcd.setCursor(12, 0); lcd.print(label1);
  lcd.setCursor(13, 0); lcd.print(max_value);
  lcd.setCursor(12, 1); lcd.print(label2);
  lcd.setCursor(13, 1); lcd.print(min_value);
#endif
  for (byte i = 0; i < width; i++) {                  // каждый столбец параметров
    int fill_val = plot_array[i];
    fill_val = constrain(fill_val, min_val, max_val);
    byte infill, fract;
    // найти количество целых блоков с учётом минимума и максимума для отображения на графике
    if ((plot_array[i]) > min_val)
      infill = floor((float)(plot_array[i] - min_val) / (max_val - min_val) * height * 10);
    else infill = 0;
    fract = (float)(infill % 10) * 8 / 10;            // найти количество оставшихся полосок
    infill = infill / 10;

    for (byte n = 0; n < height; n++) {     // для всех строк графика
      if (n < infill && infill > 0) {       // пока мы ниже уровня
        lcd.setCursor(i, (row - n));        // заполняем полными ячейками
        lcd.write(255);
      }
      if (n >= infill) {                    // если достигли уровня
        lcd.setCursor(i, (row - n));
        if (n == 0 && fract == 0) fract++;      // если нижний перел графика имеет минимальное значение, то рисуем одну полоску, чтобы не было пропусков (с)НР
        if (fract > 0) lcd.write(fract);        // заполняем дробные ячейки
        else lcd.write(16);                     // если дробные == 0, заливаем пустой
        for (byte k = n + 1; k < height; k++) { // всё что сверху заливаем пустыми
          lcd.setCursor(i, (row - k));
          lcd.write(16);
        }
        break;
      }
    }
  }
}

void loadClock() {
  DateTime now=rtc.now();
  if (bigDig && (DISPLAY_TYPE == 1)) {              // для четырехстрочных цифр (с)НР
    lcd.createChar(0, UT);
    lcd.createChar(1, row3);
    lcd.createChar(2, UB);
    lcd.createChar(3, row5);
    lcd.createChar(4, KU);
    lcd.createChar(5, KD);
  }
  else {                                            // для двустрочных цифр (с)НР
    lcd.createChar(0, row2);
    lcd.createChar(1, UB);
    lcd.createChar(2, row3);
    lcd.createChar(3, UMB);
    lcd.createChar(4, LMB);
    lcd.createChar(5, LM2);
  }

  if (now.dayOfTheWeek() == 4)  {          // Для четверга в ячейку запоминаем "Ч", для субботы "Б", иначе "П" (с)НР
    lcd.createChar(7, CH);  // Ч (с)НР
  } else if (now.dayOfTheWeek() == 6) {
    lcd.createChar(7, BB);  // Б (с)НР
  } else {
    lcd.createChar(7, PP);  // П (с)НР
  }
}

void loadPlot() {
  lcd.createChar(0, rowS);      // Стрелка вниз для индикатора пределов (с)НР
  lcd.createChar(1, row1);
  lcd.createChar(2, row2);
  lcd.createChar(3, row3);
  lcd.createChar(4, row4);
  lcd.createChar(5, row5);
  lcd.createChar(6, row6);
  lcd.createChar(7, row7);
}

#if (LED_MODE == 0)
byte LED_ON = (LED_BRIGHT_MAX);
byte LED_OFF = (0);
#else
byte LED_ON = (255 - LED_BRIGHT_MAX);
byte LED_OFF = (255);
#endif

void setLEDcolor(byte color) {                    // цвет индикатора задается двумя битами на каждый цвет (с)НР
  analogWrite(LED_R, LED_ON + LED_ON * ((LED_MODE << 1) - 1) * (3 - (color & 3)) / 3);
  analogWrite(LED_G, LED_ON + LED_ON * ((LED_MODE << 1) - 1) * (3 - ((color & 12) >> 2)) / 3);
  analogWrite(LED_B, LED_ON + LED_ON * ((LED_MODE << 1) - 1) * (3 - ((color & 48) >> 4)) / 3);
}

void setLED() {

  if (LED_BRIGHT < 11) {                                     // если ручные установки яркости
    LED_ON = 255 / 100 * LED_BRIGHT * LED_BRIGHT;
  } else {
    checkBrightness();
  }
  if (LED_MODE != 0) LED_ON = 255 - LED_ON;

  // ниже задается цвет индикатора в зависимости от назначенного сенсора: красный, желтый, зеленый, синий (с)НР

  if ((dispCO2 >= maxCO2) && LEDType == 0 || (dispHum <= minHum) && LEDType == 1 || (dispTemp >= maxTemp) && LEDType == 2 || (dispRain <= minRain) && LEDType == 3 || (dispPres <= minPress) && LEDType == 4) setLEDcolor(3);   // красный
  else if ((dispCO2 >= normCO2) && LEDType == 0 || (dispHum <= normHum) && LEDType == 1 || (dispTemp >= normTemp) && LEDType == 2 || (dispRain <= normRain) && LEDType == 3 || (dispPres <= normPress) && LEDType == 4) setLEDcolor(3 + 8);   // желтый
  else if (LEDType == 0 || (dispHum <= maxHum) && LEDType == 1 || (dispTemp >= minTemp) && LEDType == 2 || (dispRain <= maxRain) && LEDType == 3 || LEDType == 4) setLEDcolor(12);    // зеленый
  else setLEDcolor(48);   // синий (если влажность превышает заданный максимум, температура ниже минимума, вероятность осадков выше maxRain)
}

void setup() {
  EEPROM.begin(16);
  Serial.begin(115200);

  pinMode(BACKLIGHT, OUTPUT);
  pinMode(LED_COM, OUTPUT);
  pinMode(LED_R, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  setLEDcolor(0);

  digitalWrite(LED_COM, LED_MODE);
  analogWrite(BACKLIGHT, LCD_BRIGHT_MAX);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  Serial.println("Mounting FS...");
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount file system");
  }

  if (EEPROM.read(0) & 0b1000) {      // если было сохранение настроек, то восстанавливаем их
    Serial.println("Restore settings..");
    lcd.print("Restore settings..");
    MAX_ONDATA = EEPROM.read(2);
    MAX_ONDATA += (long)(EEPROM.read(3) << 8);
    VIS_ONDATA = EEPROM.read(4);
    VIS_ONDATA += (long)(EEPROM.read(5) << 8);
    mode0scr = EEPROM.read(6);
    bigDig = EEPROM.read(7);
    LED_BRIGHT = EEPROM.read(8);
    LCD_BRIGHT = EEPROM.read(9);
    LEDType = EEPROM.read(10);
  }
  
  Serial.println("");
  loadConfig();
  Serial.print("TIMEZONE: ");
  Serial.println(String(TIMEZONE));
  Serial.print("mqtt_ip: ");
  Serial.println(mqtt_ip);
  Serial.print("mqtt_port: ");
  Serial.println(mqtt_port);
  Serial.print("mqtt_auth: ");
  Serial.println(mqtt_auth);
  Serial.print("mqtt_pass: ");
  Serial.println(mqtt_pass);

  if( ssid.length() == 0 ) ssid = "*";
  if( pass.length() == 0 ) pass = "*";
  Serial.print("ssid: ");
  Serial.println(ssid);
  Serial.print("pass: ");
  Serial.println(pass);
  Serial.print("otaFlag: ");
  Serial.println(otaFlag);
  if (!otaFlag) {
    if (mqtt_server.fromString(mqtt_ip)) { //Указан корректный ip адрес
      String ip = mqtt_ip;
      unsigned int pos = ip.indexOf(".");
      unsigned int pos_last = ip.lastIndexOf(".");
      ip_addr[0] = ip.substring(0,pos).toInt();
      ip_addr[3] = ip.substring(pos_last+1).toInt();
      ip    = ip.substring(pos+1,pos_last);
      pos = ip.indexOf(".");
      ip_addr[1] = ip.substring(0,pos).toInt();
      ip_addr[2] = ip.substring(pos+1).toInt();
      IPAddress mqtt_server(ip_addr[0],ip_addr[1],ip_addr[2],ip_addr[3]);
      client.set_server(mqtt_server,mqtt_port.toInt()); 
      Serial.print("MQTT IP: "); Serial.println(mqtt_server);
      Serial.print("MQTT PORT: "); Serial.println(mqtt_port);
    } 
    else {client.set_server(mqtt_ip,mqtt_port.toInt()); Serial.print("MQTT name: "); Serial.println(mqtt_ip); Serial.print("MQTT PORT: "); Serial.println(mqtt_port);}
  
    delay(1000);
    lcd.clear();
    boolean status = true;
    setLEDcolor(3);

#if (CO2_SENSOR == 1)
    lcd.setCursor(0, 0);
    lcd.print(F("MHZ-19... "));
    Serial.print(F("MHZ-19... "));
    mhz19.begin(MHZ_TX, MHZ_RX);
    mhz19.setAutoCalibration(false);
    mhz19.getStatus();    // первый запрос, в любом случае возвращает -1
    delay(500);
    if (mhz19.getStatus() == 0) {
      lcd.print(F("OK"));
      Serial.println(F("OK"));
    } else {
      lcd.print(F("ERROR"));
      Serial.println(F("ERROR"));
      status = false;
    }
#endif

    setLEDcolor(3 + 12);
    lcd.setCursor(0, 1);
    lcd.print(F("RTC... "));
    Serial.print(F("RTC... "));
    delay(50);
    if (rtc.begin()) {
      lcd.print(F("OK"));
      Serial.println(F("OK"));
    } else {
      lcd.print(F("ERROR"));
      Serial.println(F("ERROR"));
      status = false;
    }

    setLEDcolor(12);
    lcd.setCursor(0, 2);
    lcd.print(F("BME280... "));
    Serial.print(F("BME280... "));
    delay(50);
    if (bme.begin(0x76,&Wire)) {
      lcd.print(F("OK"));
      Serial.println(F("OK"));
    } else {
      lcd.print(F("ERROR"));
      Serial.println(F("ERROR"));
      status = false;
    }

    setLEDcolor(0);
    lcd.setCursor(0, 3);
    if (status) {
      lcd.print(F("All good"));
      Serial.println(F("All good"));
    } else {
      lcd.print(F("Check wires!"));
      Serial.println(F("Check wires!"));
    }
  
    for (byte i = 1; i < 10; i++) { //ожидание
      lcd.setCursor(14, 1);
      lcd.print("P:    ");
      lcd.setCursor(16, 1);
      lcd.print(analogRead(PHOTO), 1);
      Serial.println(analogRead(PHOTO));
      delay(200);
    }
  } //!otaFlag
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Connecting to WiFi"));
  lcd.setCursor(0, 1);
  lcd.print(ssid);
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.persistent(false);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  uint8_t mac[6];
  for (byte i = 1; i < 100; i++) {
    delay(100);
    Serial.print(".");
    if ( WiFi.status() == WL_CONNECTED ) break;
  }
  if ( WiFi.status() == WL_CONNECTED ) {
    lcd.print(F(" OK"));
    Serial.print("IP number assigned by DHCP is ");
    Serial.println(WiFi.localIP());
    /*Serial.println("Starting UDP");
    Udp.begin(localPort);
    Serial.print("Local port: ");
    Serial.println(Udp.localPort());
    Serial.println("waiting for sync");
    setSyncProvider(getNtpTime);
    setSyncInterval(60);*/
    ntp.begin();
    if (ntp.synced())
      {
        SetTimeDateAndDate(ntp.second(),ntp.minute(),ntp.hour(),ntp.dayWeek(),ntp.day(),ntp.month(),ntp.year());
      }
    WiFi.macAddress(mac);
  }
  else {
    lcd.clear();
    lcd.print(F("Not connected to WiFi"));
    Serial.println("Not connected to WiFi ");
  }

  if ( WiFi.status() != WL_CONNECTED ) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(clock_ssid, clock_pass);
    sprintf(myIP, "%s", WiFi.softAPIP().toString().c_str());
    Serial.print("WiFi SSID: ");
    Serial.println(clock_ssid);
    Serial.print("WiFi password: ");
    Serial.println(clock_pass);
    lcd.setCursor(0, 3);
    lcd.print(clock_ssid);
    WiFi.softAPmacAddress(mac);
  }                   
  else {
    sprintf(myIP, "%s", WiFi.localIP().toString().c_str());
  }
  lcd.setCursor(0, 2);
  lcd.print(myIP);

  hostName += "-";
  hostName += macToStr(mac);
  hostName.replace(":","-");
  host = (char*) hostName.c_str();
  Serial.print("host: "); Serial.println(hostName.c_str());
  otaFlag_ = otaFlag;
  if( otaFlag ) {
    lcd.setCursor(0, 3);
    lcd.print("Updating...");
    setOtaFlag(0);
    handleOTA();
  }
  else {
    MDNS.begin(host);
    Serial.print("Open http://");
    Serial.print(myIP);
    Serial.println("/ in your browser");
    server.on("/set_clock", SetTime);
    server.on("/", HandleClient);
    server.on("/set_WI_FI", handleRoot);
    server.on("/ok", handleOk);
    server.on("/pageselect", handlePageSelect);
    server.begin();
    MDNS.addService("http", "tcp", 80);
    Serial.printf("Ready! Open http://%s.local in your browser\n", host);
      
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                   Adafruit_BME280::SAMPLING_X1, // temperature
                   Adafruit_BME280::SAMPLING_X1, // pressure
                   Adafruit_BME280::SAMPLING_X1, // humidity
                   Adafruit_BME280::FILTER_OFF);
  
    if (RESET_CLOCK || rtc.lostPower()) {
      // When time needs to be set on a new device, or after a power loss, the
      // following line sets the RTC to the date & time this sketch was compiled
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      // This line sets the RTC with an explicit date & time, for example to set
      // January 21, 2014 at 3am you would call:
      // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    }

    DateTime now = rtc.now();
    secs = now.second();
    mins = now.minute();
    hrs = now.hour();

    bme.takeForcedMeasurement();
    uint32_t Pressure = bme.readPressure();
    for (byte i = 0; i < 6; i++) {          // счётчик от 0 до 5
      pressure_array[i] = Pressure; // забить весь массив текущим давлением
      //time_array[i] = i;                    // забить массив времени числами 0 - 5
    }

    dispAlt = (float)bme.readAltitude(SEALEVELPRESSURE_HPA);

    // заполняем графики текущим значением
    readSensors();
    for (byte i = 0; i < 15; i++) {   // счётчик от 0 до 14
      tempHour[i] = dispTemp;
      tempDay[i] = dispTemp;
      humHour[i] = dispHum;
      humDay[i] = dispHum;
      //    rainHour[i] = 0;
      //    rainDay[i] = 0;
      altHour[i] = dispAlt;
      altDay[i] = dispAlt;
      if (PRESSURE) {
        pressHour[i] = 0;
        pressDay[i] = 0;
      } else {
        pressHour[i] = dispPres;
        pressDay[i] = dispPres;
      }
    }

    delay(2500);
    lcd.clear();
    if (DISPLAY_TYPE == 1) drawData();
    loadClock();
    // readSensors();
    drawSensors();
  }
}

void loop() {
  ntp.tick();
  if (WiFi.status() == WL_CONNECTED && otaFlag_){
    if(otaCount<=1) {
      Serial.println("OTA mode time out. Reset!"); 
      setOtaFlag(0);
      delay(1000);
      ESP.reset();
      delay(100);
    }
    server.handleClient();
    delay(1);
  }
  else {
    if (testTimer(brightTimerD, brightTimer)) checkBrightness();  // яркость
    if (testTimer(sensorsTimerD, sensorsTimer)) readSensors();    // читаем показания датчиков с периодом SENS_TIME
    if (testTimer(clockTimerD, clockTimer)) clockTick();          // два раза в секунду пересчитываем время и мигаем точками
    plotSensorsTick();                                            // тут внутри несколько таймеров для пересчёта графиков (за час, за день и прогноз)
    modesTick();                                                  // тут ловим нажатия на кнопку и переключаем режимы
    if (mode == 0) {  // в режиме "главного экрана"
      if (testTimer(drawSensorsTimerD, drawSensorsTimer)) drawSensors();  // обновляем показания датчиков на дисплее с периодом SENS_TIME
    } else {                                               // в любом из графиков
      if (testTimer(plotTimerD, plotTimer)) redrawPlot();  // перерисовываем график
    } 
    if (WiFi.status() == WL_CONNECTED) {
      if (testTimer(RTCsyncTimerD, RTCsyncTimer)) if (ntp.synced()) rtc.adjust(DateTime(ntp.year(), ntp.month(), ntp.day(), ntp.hour(), ntp.minute(), ntp.second()));
      if (testTimer(WEBsyncTimerD, WEBsyncTimer)) server.handleClient();
       if (testTimer(ntpTimerD, ntpTimer)) 
       {
        if (ntp.synced())
          {
            SetTimeDateAndDate(ntp.second(),ntp.minute(),ntp.hour(),ntp.dayWeek(),ntp.day(),ntp.month(),ntp.year());
          }
       }
      if (testTimer(MQTTsyncTimerD, MQTTsyncTimer)) {
        if (!client.connected()) {
          Serial.println("Connecting to MQTT server");
          if (client.connect(MQTT::Connect("espClient").set_auth(mqtt_auth, mqtt_pass))) Serial.println("Connected to MQTT server");
          else Serial.println("Could not connect to MQTT server");
        }   
        if (client.connected()) {
          client.loop();
          client.publish(mqtt_Temp, String (dispTemp,1));
          client.publish(mqtt_Hum, String (dispHum));
          client.publish(mqtt_Press, String (dispPres));
          client.publish(mqtt_CO2, String (dispCO2));   
          Serial.println("Publish to MQTT server");
        }
      }
    }
  }
}
