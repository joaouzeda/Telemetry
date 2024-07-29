/*----------------------------------------------------------------

  Telemetry V0.8.1 main.cpp
     
  INA226
  MFRC522 
  LCD 20x4 I²c 
  Keypad 4x4 I²c 

  Compiler: VsCode 1.9.0
  MCU: ESP32
  Board: Dev module 38 pins

  Author: João  G. Uzêda & Victor Hugo
  date: 2024, June
-----------------------------------------------------------------*/

// ---------------------------------------------------------------- 
// ---Libraries---
#include "freertos/FreeRTOSConfig.h"
#include "LiquidCrystal_I2C.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "PubSubClient.h"
#include "Preferences.h"
#include "esp_system.h"
#include "I2CKeyPad.h"
#include "Password.h"
#include "rtc_wdt.h"
#include "Arduino.h"
#include "MFRC522.h"
#include "INA226.h"
#include "stdio.h"
#include "WiFi.h"
#include "Wire.h"
#include "LoRa.h"
#include "SPI.h"

// ---------------------------------------------------------------- 
// ---LoRa Settings---
#define SF 12
#define FREQ 915E6
#define SWB 41.7E3
#define SYNC_WORD 0x34
#define TX_POWER_DB 17

// ---------------------------------------------------------------- 
// ---defines---
#define SHUNT_RESISTENCE  0.75

// ----------------------------------------------------------------  
//----I²C Adresses------
#define LCDADRESS 0x27 
#define INAADRESS 0x40 

// ----------------------------------------------------------------  
//----pins------
#define   RFID_SS        22
#define   RFID_RST       5
#define   RFID_SCK       14
#define   RFID_MISO      2
#define   RFID_MOSI      15
#define   INA_SDA       32
#define   INA_SCL       33 

// ----------------------------------------------------------------- 
// ---connection infos--
const char *ssid    =    "------WIFI_SSID------";             
const char *pass    =    "-----WIFI_PASSWORD---";   
const char *mqtt    =    "test.mosquitto.org";     
const char *user    =    "---MQTT_USER----";                           
const char *passwd  =    "--MQTT_PASSWORD-";                       
int         port    =    1883;    
// -----------------------------------------------------------------

// -----------------------------------------------------------------  
// -----Topics-----
const char *topic_JHour   =    "proto/sim/horimetro/ApenasHora";
const char *topic_FHour   =    "proto/sim/horimetro/HoraTotal";
const char *topic_HBomb   =    "proto/sim/horimetro/Elevação";
const char *topic_Htrac   =    "proto/sim/horimetro/Tração";
const char *topic_A2      =    "proto/sim/Corrente/elevacao";
const char *topic_A3      =    "proto/sim/Corrente/tracao";
const char *topic_A       =    "proto/sim/Corrente";
const char *topic_T       =    "proto/sim/tensao"; 
const char *topic_RFID    =    "proto/sim/UID";
// -----------------------------------------------------------------


// -----------------------------------------------------------------
// -----Functions----
void recon();
void readpref();
void ina226_setup();
void xTaskNav(void *pvParameters);
void xTaskTelemetry(void *pvParameters);
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// -----Objects----
Preferences pref;
WiFiClient TestClient;
MFRC522 rfid(SS, RST);
INA226_WE INA(INAADRESS);
PubSubClient client(TestClient);
LiquidCrystal_I2C lcd(LCDADRESS, 16, 2);


// -----------------------------------------------------------------
// ----Variables----
float geralA;
float ABombH;
float ATrac;
float Volt;

int hourmeter;
int hourmeterT;
int hourmeterB;
int minute;
int minuteT;
int minuteB;
int sec;
int secT;
int secB;

// -----------------------------------------------------------------
// --Preferences Key---
const char *minpref   =   "min";
const char *mintrac   =   "trac";
const char *minbomb   =   "minbomb";
const char *hourpref  =   "hour";
const char *hourtrac  =   "hourtrac";
const char *hourbomb  =   "hourbomb";

// -----------------------------------------------------------------
// --Main function---
extern "C" void app_main(){
  initArduino();
  rfid.PCD_Init();
  ina226_setup();
  Wire.begin(SDA, SCL);
  WiFi.begin(ssid, pass); 
  pref.begin("Memory", false);


  if(WiFi.status() != WL_CONNECTED)
    WiFi.reconnect();                                                                                               
  
  client.setServer(mqtt, port);           
  if (!client.connected())                            
    client.connect("TestClient", user, passwd);                                       
  
  readpref();

  xTaskCreatePinnedToCore(xTaskTelemetry, // function name
                          "Telemetry",    // task name
                          8000,           // stack size in word
                          NULL,           // input parameter
                          1,              // priority
                          NULL,           // task handle
                          1);             // core

  xTaskCreatePinnedToCore(xTaskNav,
                          "Navegation",
                          16000,
                          NULL,
                          1,
                          NULL,
                          0);

  vTaskDelay(1000);                            
}

/*------------------------------------------------------------------
--------------------------------Tasks-------------------------------
------------------------------------------------------------------*/

// -----------------------------------------------------------------
// -----telemetry-----
void xTaskTelemetry(void *pvParameters){ 
  lcd.clear(); 
  
  char CVolt[30];
  char Ageral[30];  
  char Justhour[30];
  char FullHour[30];
  char Amp_buffer[30];
  char Volt_buffer[30];

  esp_task_wdt_add(NULL);        //  enable watchdog     
  while(1){  
    rtc_wdt_feed();    //  feed watchdog 

    if (WiFi.status() != WL_CONNECTED  || !client.connected())
      recon();
    client.loop();

    INA.readAndClearFlags();

    Volt = INA.getBusVoltage_V();
    geralA = INA.getShuntVoltage_mV() / SHUNT_RESISTENCE;

    ABombH = geralA - 1.5;
    ATrac = geralA - 2.5;

    if (geralA >= 3.5){ // --------------General Hourmeter--------------------------
      sec++;   
      if (sec >= 60){         
        sec -= 60;
        minute++;
        if (minute == 10)
          pref.putInt(minpref, minute);
        else if (minute == 20)
          pref.putInt(minpref, minute);
        else if (minute == 30)
          pref.putInt(minpref, minute);
        else if (minute == 40)
          pref.putInt(minpref, minute);
        else if (minute == 50)
          pref.putInt(minpref, minute);
      }                       
      if (minute >= 60){
        minute -= 60;
        hourmeter++;
        pref.remove(minpref);
        pref.putInt(hourpref, hourmeter);
      }                     
    } // -----------------------------------------------------------------------   

    sprintf(Ageral, "{\"Corrente geral\":%.02f}", geralA);
    sprintf(CVolt, "{\"Tensao geral\":%.02f}", Volt);
    sprintf(FullHour, "{\"hourmeter\": %02d:%02d:%02d}",
                                hourmeter, minute, sec);
    sprintf(Justhour, "{\"JustHour\":%d}", hourmeter);
    sprintf(Amp_buffer, "%.02f", geralA);
    sprintf(Volt_buffer, "%.02f", Volt);
    
    client.publish(topic_A,     Ageral);
    client.publish(topic_A2,    CABombH);
    client.publish(topic_A3,    CATrac);
    client.publish(topic_T,     CVolt);
    client.publish(topic_FHour, FullHour);
    client.publish(topic_JHour, Justhour);

    esp_task_wdt_reset();        // reset watchdog if dont return any error

    client.loop();
    vTaskDelay(1000);
  }
} 
/*---------------------------------------------------------------------------------
--------------------------------FUNCTIONS----------------------------------------*/

// -----------------------------------------------------------------
// -----Reconection-----
void recon(){
  WiFi.disconnect();        
  vTaskDelay(1000);         
  WiFi.begin(ssid, pass);   
  
  vTaskDelay(1000);         
                
  client.connect("TestClient", user, passwd);     
  vTaskDelay(2500);                                       
}
// -----------------------------------------------------------------   

// -----------------------------------------------------------------
// -----INA setup-----
void ina226_setup(){
  INA.init();
  INA.setAverage(AVERAGE_4); 
  INA.setConversionTime(CONV_TIME_1100); 
  INA.setResistorRange(SHUNT_RESISTENCE, 300.0); 
  INA.waitUntilConversionCompleted(); 	
}
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// -----Read flash memory-----
void readpref(){
  minuteB     = pref.getInt(minbomb, minuteB);
  minute      = pref.getInt(minpref, minute);
  hourmeter   = pref.getInt(hourpref, hourmeter); 
  hourmeterT  = pref.getInt(hourtrac, hourmeterT);
  hourmeterB  = pref.getInt(hourbomb, hourmeterB);
}