/*----------------------------------------------------------------

  Telemetry V1.0.0 main.cpp

  Notes: 

  Features:
    1. Inserir um jeito de calcular o desvio padrão e comparar com
    o desvio padrão do efeito hall informado pelo fabricante junto 
    com a comparação da leitura do shunt e assegurar a saude do 
    sensor

      
      # float buf[10];
      # for (int i = 0; i <10; i++){
      #     buf[i] = leitura;
      #     delay(10);
      # }
      #
      # float valormedio = 0;
      # for  (int i = 0; i < 10; i++){
      #    valormedio += buf[i];
      # }    
      #
      # corrente = valormedio / 6.0
      #
      # dp = (corrente)*(1/2)

    2. Efeito Hall on/off para saber qual corrente o shunt ta lendo
    
  Notes: 

  Compiler: VsCode 1.95
  MCU: ESP32
  Espressif: 5.3.1
  Arduino Component: 3.1.0
  Board: Dev module 38 pins

  Author: João  G. Uzêda & Victor Hugo
  date: 2024, Dez
-----------------------------------------------------------------*/

// ---------------------------------------------------------------- 
// ---Libraries---
#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "Adafruit_SSD1306.h"
#include "freertos/semphr.h"
//#include "user_interface.h" 
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "Adafruit_GFX.h"
#include "Preferences.h"
#include "esp_system.h"
#include "I2CKeyPad.h"
#include "Password.h"
#include "uRTClib.h"
#include "rtc_wdt.h"
#include "Arduino.h"
#include "MFRC522.h"
#include "INA226.h"
#include "stdio.h"
#include "WiFi.h"
#include "LoRa.h"
#include "Wire.h"
#include "SPI.h"

// ---------------------------------------------------------------- 
// ---defines---
#define   SF                          12
#define   FREQ                        915E6
#define   SBW                         62.5E3	
#define   SYNC_WORD                   0x34
#define   TX_POWER_DB                 17
#define   PREAMBLE_LENGTH             8
#define   CODING_RATE_DENOMINATOR     5

#define   SHUNT_RESISTENCE            0.75
#define   Time_One                    2
#define   Time_Two                    16
#define   Time_Three                  32
#define   Time_Four                   48
#define   Task_Real_Sync_Flag         (1<<0)
#define   SCREEN_WIDTH                128 
#define   SCREEN_HEIGHT               64 
#define   GENERAL_CURRENT_BASE        3.5
#define   HIDRAULIC_BOMB_BASE_CURRENT 13
#define   TRACTION_BASE_CURRENT       13
#define   SAFE_POINT_MINUTE_1         10
#define   SAFE_POINT_MINUTE_2         20
#define   SAFE_POINT_MINUTE_3         30
#define   SAFE_POINT_MINUTE_4         40
#define   SAFE_POINT_MINUTE_5         50
#define   SAFE_POINT_MINUTE_6         60

char keypad[19] = "DCBA#9630852*741NF";

// ----------------------------------------------------------------  
//----I²C Adresses------
#define INA_ADRESS      0x40 
#define Key_Adress      0x38
#define RTC_ADREED      0x68
#define SCREEN_ADDRESS  0x3C 

// ----------------------------------------------------------------  
//----pins------
#define   RFID_CS       2
#define   RFID_RST      22

#define   RFID2_CS      15
#define   RFID2_RST     22

#define   SPI_SCK       14
#define   SPI_MISO      12
#define   SPI_MOSI      13

#define   I2C_SDA       32
#define   I2C_SCL       33 

#define   LORA_CS       5
#define   LORA_RST      0 

#define   KEY_PIN       17  // sinal em cunjunto com o lat para manter a placa ligada quando a chave é desligada pino 3 do conector 4 vias
#define   OUT_PIN       25  // liga a maquina pino 2 do conector 4 vias
#define   LAT_PIN       21  // high mantem a placa ligado independente da chave 

#define   IMU_SDA       27     
#define   IMU_SCL       26

#define   OLED_RESET     -1 

// ----------------------------------------------------------------
// -----Tasks----
void xTaskTelemetry(void *pvParameters);
void xTaskNav(void *pvParameters);
void xTaskRealSync(void *pvParameters);
void CallBackTimer(TimerHandle_t pxTimer);

// ---------------------------------------------------------------- 
// ---Handles/Events---
TimerHandle_t xTimer;
TaskHandle_t xHandleRSync;
TaskHandle_t xHandleHourmeter;
TaskHandle_t xHandleTelemetry;
EventGroupHandle_t xEvents;

// ----------------------------------------------------------------
// -----Func. Prototype----
void CadastrarCartao(String tipoSelecionado);
void erease(char key, int buffer);
void tag(char key, int buffer);
void dell(int buffer);
void resetPassword();
void aprovadoPass();
void ina226_setup();
void Keypadconfig();
void emergencia();
void RFIDConfig();
void manutencao();
void LoRaconfig();
void telafinal();
void cadastrar();
void vazamento();
void hourcheck();
void RTCConfig();
void readpref();
void formatar();
void Taskconf();
void screens();
void bateria();
void comando();
void excluir();
void status();
void garfos();
void format();
void telas();
void input();
void recon();
void eng();
void apx();

// ----------------------------------------------------------------
// -----Objects----
uRTCLib rtc;
Preferences pref;
INA226_WE INA(INA_ADRESS);
I2CKeyPad kpd(Key_Adress);
MFRC522 rfid(RFID_CS, RFID_RST);
MFRC522 rfid2(RFID2_CS, RFID2_RST);
Password password = Password((char*)"2552");
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// ----------------------------------------------------------------
// ----Variables----
struct Hourmeter {
  unsigned Hour, minute, sec;
} TractionEngine, HidraulicBomb, General;

struct Answers{
  unsigned Answer1 : 1;
  unsigned Answer2 : 1;
  unsigned Answer3 : 1;
  unsigned Answer4 : 1;
  unsigned Answer5 : 1;
} Ans;

float GeneralCurrent;
float CurrentHidraulicBomb;
float CurrentTractionEngine;
float Voltage;

int Tempo_de_Envio;

// ----------------------------------------------------------------
// ----Constants----
byte a = 45;
byte b = 30;
byte c = 30;
byte maxtaglen = 6; 
byte maxpasslen = 5;  
uint8_t MID = 150;

// ----------------------------------------------------------------
// ----Buffers----
byte currentpasslen = 0;  
byte currenttaglen = 0;
uint8_t priority = 0;
char uid_buffer[32];
char uid_bufferOpe[32];
char uid_battery[32];
char CAD[32];
int  manup =  0; 
int  manuc =  0; 
bool psswdcheck;
int  turnosalvo;
int  turnoatual;
bool opnav;
int  Hora;

// ----------------------------------------------------------------
// ---Preferences Key---
const char *Opepref         =   "CadastroOpe";
const char *Tecpref         =   "CadastroTec";
const char *Admpref         =   "CadastroAdm";
const char *prevpref        =   "Manupreve";
const char *correpref       =   "Manucorre";
const char *cadaspref       =   "Cadastro";
const char *Turnopref       =   "TurnoOpe";
const char *HourmeterPreference           =   "hour";
const char *GenerealMinutePreference      =   "min";
const char *MinuteEnginePreference        =   "trac";
const char *MinuteHidraulicPreference     =   "minbomb";
const char *HourmeterEnginePreference     =   "hourtrac";
const char *HourmeterHidraulicPreference  =   "hourbomb";

// ----------------------------------------------------------------
// -----UIDS-----
struct Cartoes{
  String uid  = "" ;
}Operador, Tecnico, Adm;

// ----------------------------------------------------------------
// -----Main Func-----
extern "C" void app_main(){
  initArduino();
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  ina226_setup();
  RFIDConfig();
  LoRaconfig();

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)){
    Serial.println(F("SSD1306 allocation failed"));
  }
  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);

  pinMode(KEY_PIN, INPUT);
  pinMode(OUT_PIN, OUTPUT);
  pinMode(LAT_PIN, OUTPUT);

  digitalWrite(LAT_PIN, HIGH);

  pref.begin("GT", false);

  RTCConfig();
  Keypadconfig();
  readpref();

  display.setCursor(25, 30);
  display.print("INICIALIZANDO");
  display.display();
  Serial.println("System Initizalized");

  Taskconf();
  vTaskDelay(2500);
  display.clearDisplay();     
}

/*---------------------------------------------------------------------------------
--------------------------------------Tasks----------------------------------------
---------------------------------------------------------------------------------*/

// -----------------------------------------------------------------
// -----telemetry-----
void xTaskTelemetry(void *pvParameters){            
  esp_task_wdt_add(NULL);
  while(1){  
    rtc_wdt_feed();   
    INA.readAndClearFlags();

    Voltage = INA.getBusVoltage_V();
    GeneralCurrent = INA.getShuntVoltage_mV() / SHUNT_RESISTENCE;

    if (GeneralCurrent >= 3.5){ 
      General.sec++;   
      if (General.sec >= 60){         
        General.sec -= 60;
        General.minute++;
        if (General.minute == 10)
          pref.putInt(GenerealMinutePreference, General.minute);
        else if (General.minute == 20)
          pref.putInt(GenerealMinutePreference, General.minute);
        else if (General.minute == 30)
          pref.putInt(GenerealMinutePreference, General.minute);
        else if (General.minute == 40)
          pref.putInt(GenerealMinutePreference, General.minute);
        else if (General.minute == 50)
          pref.putInt(GenerealMinutePreference, General.minute);
      }                       
      if (General.minute >= 60){
        General.minute -= 60;
        General.Hour++;
        priority = 1;
        pref.remove(GenerealMinutePreference);
        pref.putInt(HourmeterPreference, General.Hour);
      }                     
    } 
    esp_task_wdt_reset();       
    vTaskDelay(500);
  }
} 

// -----------------------------------------------------------------
// -----Navegation task-----
void xTaskNav(void *pvParameters){ 
  esp_task_wdt_add(NULL);
  while(1){         
    char menu = kpd.getChar();
    if (manup == 1) {
      status();
    } else if (manuc == 1) {
      status();
    } else {
      esp_task_wdt_reset(); 
      vTaskDelay(250);
      if (menu != 'N') {
        vTaskDelay(90);
        if (menu == '0') {
          opnav = false; 
          esp_task_wdt_reset(); 
          vTaskDelay(80);
          eng();        
        }else {
          opnav = true; 
             
          vTaskDelay(250); 
          apx();
        }
      }else if(menu == 'F'){
        Serial.println("Erro no keypad!");
      }else {
        opnav = true;
        esp_task_wdt_reset();  
        vTaskDelay(250);   
        apx();
      }
    }
    esp_task_wdt_reset();  
    vTaskDelay(500);
  }
}

// -----------------------------------------------------------------
// -----Real Sync task-----
void xTaskRealSync(void *pvParameters){
  EventBits_t xBits;
  while(1){
    xBits = xEventGroupWaitBits(xEvents, Task_Real_Sync_Flag, pdTRUE, pdTRUE, portMAX_DELAY);
    digitalWrite(LORA_CS, LOW); 
    if(priority == 0){
      LoRa.beginPacket();
      LoRa.print(priority);
      LoRa.print(" ");
      LoRa.print(MID);
      LoRa.print(" ");
      LoRa.print(GeneralCurrent);
      LoRa.print(" ");
      LoRa.print(Voltage);
      LoRa.print(" ");
      LoRa.endPacket();
    }else if(priority == 1){
      LoRa.beginPacket();
      LoRa.print(priority);
      LoRa.print(" ");
      LoRa.print(MID);
      LoRa.print(" ");
      LoRa.print(General.Hour);
      LoRa.print(" ");
      LoRa.endPacket();
      priority = 0;
    }else if(priority == 2){
      LoRa.beginPacket(); 
      LoRa.print(priority);
      LoRa.print(" ");
      LoRa.print(MID);
      LoRa.print(" ");
      LoRa.print(uid_buffer);
      LoRa.print(" ");
      LoRa.print(uid_battery);
      LoRa.print(" ");
      LoRa.print(Ans.Answer1);
      LoRa.print(" ");
      LoRa.print(Ans.Answer2);
      LoRa.print(" ");
      LoRa.print(Ans.Answer3);
      LoRa.print(" ");
      LoRa.print(Ans.Answer4);
      LoRa.print(" ");
      LoRa.print(Ans.Answer5);
      LoRa.print(" ");
      LoRa.endPacket(); 
      priority = 0;
    }
    //rtc_wdt_feed();
    //esp_task_wdt_reset(); 
    vTaskDelay(50);
  }
}

/*---------------------------------------------------------------------------------
-------------------------------------Functions-------------------------------------
---------------------------------------------------------------------------------*/

// -----------------------------------------------------------------
// -----LCD config-----
void Taskconf(){
  xEvents = xEventGroupCreate();
  xTimer = xTimerCreate("Timer", 
                        pdMS_TO_TICKS(1000), 
                        pdTRUE, 
                        0, 
                        CallBackTimer);

  xTaskCreatePinnedToCore(xTaskTelemetry,                   // function name
                          "Telemetry",                      // task name
                          configMINIMAL_STACK_SIZE + 2048,  // stack size in word
                          NULL,                             // input parameter
                          3,                                // priority
                          &xHandleTelemetry,                // task handle
                          1);                               // core

  xTaskCreatePinnedToCore(xTaskNav,
                          "Navegation",
                          configMINIMAL_STACK_SIZE + 2048,
                          NULL,
                          1,
                          NULL,
                          1);

  xTaskCreatePinnedToCore(xTaskRealSync,
                          "RealSync",
                          configMINIMAL_STACK_SIZE,
                          NULL,
                          2,
                          &xHandleRSync,
                          1);

  xTimerStart(xTimer, 0);
  Serial.println("RTOS Configured!");
}

//  ---------------------------------------------------------------
//  -----RTC config-----
void RTCConfig(){
  URTCLIB_WIRE.begin(I2C_SDA, I2C_SCL);
  rtc.set_model(URTCLIB_MODEL_DS3231);
  rtc.set_rtc_address(0x68);
  rtc.enableBattery(); 
  Serial.println("RTC configured!");
}

// -----------------------------------------------------------------
// -----LoRa config-----
void LoRaconfig(){
  LoRa.setPins(LORA_CS, LORA_RST);

  pinMode(LORA_CS, OUTPUT);
  pinMode(LORA_RST, OUTPUT);

  if (!LoRa.begin(FREQ)){
    LoRa.begin(FREQ);
    Serial.println("Starting LoRa failed!");
  }else Serial.println("LoRa Initizalized!");

  LoRa.setSyncWord(SYNC_WORD);
  LoRa.setSpreadingFactor(SF);
  LoRa.setSignalBandwidth(SBW);
  LoRa.setTxPower(TX_POWER_DB); 
  LoRa.setPreambleLength(PREAMBLE_LENGTH);
  LoRa.setCodingRate4(CODING_RATE_DENOMINATOR);
  
}

// -----------------------------------------------------------------
// -----Keypad config-----
void Keypadconfig(){
  kpd.begin();
  kpd.setKeyPadMode(I2C_KEYPAD_4x4);
  kpd.loadKeyMap(keypad);
} 

// ---------------------------------------------------------------- 
// ---CallBack---
void CallBackTimer(TimerHandle_t pxTimer){
  rtc.refresh(); 
  Tempo_de_Envio = rtc.second();
  Serial.printf("%d:%d:%d\n", rtc.hour(), rtc.minute(), rtc.second());
  if (Tempo_de_Envio == Time_One || Tempo_de_Envio == Time_One || Tempo_de_Envio == Time_Three || Time_Four){
    xEventGroupSetBits(xEvents, Task_Real_Sync_Flag);

  }
}

// -----------------------------------------------------------------
// -----RFID config-----
void RFIDConfig(){
  rfid.PCD_Init();
  rfid2.PCD_Init();
  Serial.println("RFID Initizalized");
}

// -----------------------------------------------------------------
// -----read flash memory-----
void readpref(){
  General.Hour       =  pref.getInt(HourmeterPreference, General.Hour); 
  General.minute     =  pref.getInt(GenerealMinutePreference, General.minute);
  TractionEngine.Hour      =  pref.getInt(HourmeterEnginePreference, TractionEngine.Hour);
  TractionEngine.minute    =  pref.getInt(MinuteEnginePreference, TractionEngine.minute);
  HidraulicBomb.Hour       =  pref.getInt(MinuteHidraulicPreference, HidraulicBomb.Hour);
  HidraulicBomb.minute     =  pref.getInt(HourmeterHidraulicPreference, HidraulicBomb.minute);
  manuc            =  pref.getInt(correpref, manuc);
  manup            =  pref.getInt(prevpref, manup);
  turnosalvo       =  pref.getInt(Turnopref, turnosalvo);
  Operador.uid     =  pref.getString(Opepref, Operador.uid);
  Tecnico.uid      =  pref.getString(Tecpref, Tecnico.uid);
  Adm.uid          =  pref.getString(Admpref, Adm.uid);
}

// -----------------------------------------------------------------
// -----cadastro-----
void CadastrarCartao(String tipoSelecionado) {
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    snprintf(CAD, sizeof(CAD), "%02X%02X%02X%02X",
              rfid.uid.uidByte[0], rfid.uid.uidByte[1],
              rfid.uid.uidByte[2], rfid.uid.uidByte[3]);

      // Verifica se o cartão já está cadastrado
    if (Operador.uid.indexOf(CAD) != -1 || Tecnico.uid.indexOf(CAD) != -1 || Adm.uid.indexOf(CAD) != -1) {
      // Cartão já cadastrado
      display.clearDisplay();
      display.setCursor(30, 20);
      display.print("CARTAO");
      display.setCursor(35, 30);
      display.print("JA");
      display.setCursor(20, 40);
      display.print("CADASTRADO!");
      display.display();
      esp_task_wdt_reset(); 
      vTaskDelay(250);
      display.clearDisplay();
      cadastrar(); 
    } else {
      // Adiciona o cartão à lista apropriada
      if (tipoSelecionado == "TECNICO") {
        if (Tecnico.uid.length() > 0) {
          Tecnico.uid+= ";"; 
        }
        Tecnico.uid += CAD;
        pref.putString(Tecpref, Tecnico.uid); 
      } else if (tipoSelecionado == "ADMINISTRADOR") {
        if (Adm.uid.length() > 0) {
          Adm.uid += ";"; 
        }
        Adm.uid += CAD;
        pref.putString(Admpref, Adm.uid); 
      } else {
        if (Operador.uid.length() > 0) {
          Operador.uid += ";"; 
        }
        Operador.uid += CAD;
        pref.putString(Opepref, Operador.uid); 
      }

      Serial.println("Lista de operadores cadastrados escrita na EEPROM:");
      Serial.println(Operador.uid);
      Serial.println("Lista de técnicos cadastrados escrita na EEPROM:");
      Serial.println(Tecnico.uid);
      Serial.println("Lista de administradores cadastrados escrita na EEPROM:");
      Serial.println(Adm.uid);

      esp_task_wdt_reset(); 
      display.setCursor(6, 2);
      display.print(CAD);
      display.clearDisplay();
      display.setCursor(50, 40);
      display.print("CARTAO CADASTRADO");
      display.display();
      vTaskDelay(250);

      display.clearDisplay();
      cadastrar();
    }
  }    
}

// -----------------------------------------------------------------
// -----Erease uid list-----
void format(){
  display.clearDisplay();
  display.setCursor(5, 2);
  display.print("FORMATADO");
  vTaskDelay(1000); 
  
  Operador.uid = "";
  pref.putString(Opepref, Operador.uid);
  Tecnico.uid= "";
  pref.putString(Tecpref, Tecnico.uid);
  Adm.uid = "";
  pref.putString(Admpref, Adm.uid); 
}

// -----------------------------------------------------------------
// -----Reconection-----
void recon(){
  LoRa.begin(FREQ);  
  Serial.println("Reconectando...");      
}

// -----------------------------------------------------------------
// -----INA setup-----
void ina226_setup(){
  INA.init();
  INA.setAverage(AVERAGE_4); 
  INA.setConversionTime(CONV_TIME_1100); 
  INA.setResistorRange(SHUNT_RESISTENCE, 300.0); 
  INA.waitUntilConversionCompleted(); 	
  Serial.println("INA Initizalized");
}

// -----------------------------------------------------------------
// -----dell-----
void dell(int buffer){
  display.clearDisplay();
  if(buffer == 1){
    currenttaglen = 0;
    b = 5;
    cadastrar();
  }else if (buffer == 2){
    c = 10;
    input();
  }
}

// -----------------------------------------------------------------
// -----tag-----
void tag(char key, int buffer){
  if (buffer == 1){
    display.setCursor(b, 40);
    display.print(key);
    display.display();
    b = b + 6;

    if (b == 54){
      b = 30;  
    }

    currenttaglen++;

  }else if(buffer == 0){
    display.setCursor(a, 40);
    display.print("*");
    display.display();
    a = a + 6;

    if (a == 11) 
      a = 30; 
    
    currentpasslen++;
    password.append(key);

    if (currentpasslen == maxpasslen) 
      aprovadoPass();
    
  }else if(buffer == 2){
    display.setCursor(c, 40);
    display.print(key);
    display.display();
    c = c + 6;

    if (c == 16)
      c = 10;
  }
}
  
// -----------------------------------------------------------------
// -----resetpsswd-----
void resetPassword(){
  password.reset();
  currentpasslen = 0;
  display.clearDisplay();
  a = 45;

  if(psswdcheck != true)
    eng(); 
}

// -----------------------------------------------------------------
// -----aprovadoPass-----
void aprovadoPass(){
  currentpasslen = 0;

  if (password.evaluate()) {
    display.clearDisplay();
    display.setCursor(50, 40);
    display.print("VALIDO");
    display.display();
    esp_task_wdt_reset();
    vTaskDelay(250);
    a = 45;
    psswdcheck = true;  // mostra que psswdcheck, apaga a mensagem anterior e segue para a tela screens ou  telas
    if (opnav == true)
      screens();
    else
      telas();  
  } else {
    display.clearDisplay();
    display.setCursor(45, 30);
    display.print("INVALIDO");
    display.setCursor(25, 40);
    display.print("TENTE NOVAMENTE");
    display.display();
    esp_task_wdt_reset();
    vTaskDelay(250);
    a = 45;
    psswdcheck = false;  // mostra que psswdcheck, apaga a mensagem anterior e  volta para a tela de Password e colocar a senha correta
  }
  resetPassword();
}

// -----------------------------------------------------------------
// -----erase-----
void erease(char key, int buffer){
  if (buffer == 1){
    if (a == 7)
      a = 7;
    else {
      key = ' ';
      a--;
      display.setCursor(a, 2);
      display.print(key);
      currentpasslen--;
      vTaskDelay(20); 
    }
  }else if (buffer == 2){
    if (c == 10)
      c = 10;
    else {
      key = ' ';
      c--;
      display.setCursor(c, 0);
      display.print(key);
    }
  }else{
    if (b == 5)
      b = 5;
    else {
      key = ' ';
      b--;
      display.setCursor(b, 1);    // fazer para tag
      display.print(key);
    }
    currenttaglen--;
    vTaskDelay(20); 
  }
}

/*---------------------------------------------------------------------------------
------------------------------------Screens----------------------------------------
---------------------------------------------------------------------------------*/

// -----------------------------------------------------------------
// -----status-----
void status(){
  display.clearDisplay();
  while (1) {
    esp_task_wdt_reset();
    display.setCursor(30, 20);
    display.print("MAQUINA");
    display.setCursor(35, 30);
    display.print("EM");
    display.setCursor(30, 40);
    display.print("MANUTENCAO");

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      snprintf(uid_buffer, sizeof(uid_buffer), "%02X%02X%02X%02X",
                        rfid.uid.uidByte[0], rfid.uid.uidByte[1],
                        rfid.uid.uidByte[2], rfid.uid.uidByte[3]);
      priority = 2;

      if (Tecnico.uid.indexOf(uid_buffer) != -1) {
        manuc = 0;
        manup = 0;
        pref.putInt(correpref, manuc);
        pref.putInt(prevpref, manup);
        opnav = true;
        esp_task_wdt_reset();
        vTaskDelay(80);
        apx();
      }
    } else {
      if (manup == 1) {;
        display.setCursor(30, 50);
        display.print("PREVENTIVA");
        display.display();
        vTaskDelay(40);
      } else if (manuc == 1) {
        display.setCursor(30, 50);
        display.print("CORRETIVA");
        display.display();
        vTaskDelay(40);
      }
    }
  }
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// -----------------------------------------------------------------
// -----aprox-----
void apx(){ 
  display.clearDisplay();
  digitalWrite(RFID_CS, LOW);  
  while (opnav == true) {
    display.setCursor(15, 30);
    display.print("APROXIMAR CARTAO");
    display.display();
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      snprintf(uid_buffer, sizeof(uid_buffer), "%02X%02X%02X%02X",
                        rfid.uid.uidByte[0], rfid.uid.uidByte[1],
                        rfid.uid.uidByte[2], rfid.uid.uidByte[3]);
      esp_task_wdt_reset(); 
      if (Operador.uid.indexOf(uid_buffer) != -1) {
        display.clearDisplay();
        display.setCursor(40, 30);
        display.print("OPERADOR");
        display.setCursor(25, 40);
        display.print("IDENTIFICADO");
        display.display();
        esp_task_wdt_reset(); 
        vTaskDelay(500);     
        vazamento();    
      } else if (Tecnico.uid.indexOf(uid_buffer) != -1) {
        display.clearDisplay();
        display.setCursor(40, 30);
        display.print("TECNICO");
        display.setCursor(25, 40);
        display.print("IDENTIFICADO");
        display.display();
        esp_task_wdt_reset(); 
        vTaskDelay(500);
        manutencao();
      } else if (Adm.uid.indexOf(uid_buffer) != -1) {
        display.clearDisplay();
        display.setCursor(15, 30);
        display.print("ADMINISTRADOR");
        display.setCursor(25, 40);
        display.print("IDENTIFICADO");
        display.display();
        esp_task_wdt_reset(); 
        vTaskDelay(500);
        eng();
      }else{
        display.clearDisplay();
        display.setCursor(15, 40);
        display.print("CARTAO ERRADO");
        display.display();
        vTaskDelay(50);
        display.clearDisplay();
      }
    }
    char key = kpd.getChar();
    esp_task_wdt_reset(); 
    vTaskDelay(50);
    if (key != 'N') {  
      vTaskDelay(50);
      if (key == 'C') {
        esp_restart();
      } 
    }         
    esp_task_wdt_reset(); 
    vTaskDelay(500);
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
  }
} 

// -----------------------------------------------------------------
// -----eng screen-----
void eng(){
  display.clearDisplay();

  while (1) {
  display.setCursor(45, 30);
  display.print("SENHA:");
  display.setCursor(80, 50);
  display.print("#-SAIR");
  display.display();
    char key = kpd.getChar();
    esp_task_wdt_reset(); 
    vTaskDelay(50);
    if (key != 'N') {  
      vTaskDelay(50);
      if (key == 'C') {
        psswdcheck = false;
        resetPassword(); 
      } else if (key == '#') {
        opnav = true;
        apx();
      } else if (key == 'D') {
        aprovadoPass();
      } else if (key == 'A') {
        display.clearDisplay();
        esp_restart();
      } else if (key == '*') {
        erease(key, 1); 
      }else tag(key, 0);
    }
  }
}

// -----------------------------------------------------------------
// -----screens-----
void screens(){
  display.clearDisplay();
  display.setCursor(30, 0);
  display.print("ESCOLHA A OPCAO:");
  display.setCursor(0, 10);
  display.print("1- CADASTRAR");
  display.setCursor(0, 20);
  display.print("2- EXCLUIR");
  display.setCursor(0, 30);
  display.print("3- HORIMETRO");
  display.setCursor(80, 50);
  display.print("#-SAIR");
  display.display();
  while (opnav == true) {
    esp_task_wdt_reset();
    char key = kpd.getChar();
    vTaskDelay(20);
    if (key != 'N') {
      vTaskDelay(40);
      if (key == '1') {
        opnav = true;
        cadastrar();
      } else if (key == '2') {
        opnav = true;
        excluir();
      }else if (key == '3') 
        input();  
      else if (key == '#') {
        apx();
      }
    }
  }
}

// -----------------------------------------------------------------
// -----Telas-----
void telas(){
  display.clearDisplay();
  display.setCursor(20, 10);
  display.print("ESCOLHA A OPCAO:");
  display.setCursor(0, 20);
  display.print("1- CADASTRAR");
  display.setCursor(0, 30);
  display.print("2- FORMATAR");
  display.setCursor(0, 40);
  display.print("3- EXCLUIR");
  display.setCursor(80, 50);
  display.print("# - SAIR");
  display.display();
  while (1) {
    char key = kpd.getChar();
    esp_task_wdt_reset();
    if (key != 'N') {
      vTaskDelay(30);
      if (key == '1') 
        cadastrar();
      else if (key == '2') 
        formatar();
      else if (key == '3')
        excluir();
      else if (key == '#') {
        opnav = true;  
        apx();
      }
    }
  }
}

// -----------------------------------------------------------------
// -----Cadastrar-----
void cadastrar() {
  display.clearDisplay();
  display.setCursor(20, 30);
  display.print("TIPO:");
  display.setCursor(20, 40);
  display.print("RFID:");
  display.setCursor(80, 50);
  display.print("#-SAIR");
  display.display();
  String tipoSelecionado = ""; // Variável para armazenar o tipo selecionado

  while (1) {
    esp_task_wdt_reset();
    char key = kpd.getChar();
    vTaskDelay(70);
    if (key != 'N') {
      vTaskDelay(70);
      if (key == '1') {
        display.setCursor(60, 30);
        display.print("OPERADOR");
        display.display();
        tipoSelecionado = "OPERADOR";
      } else if (key == '2') {
        display.setCursor(60, 30);
        display.print("TECNICO");
        display.display();
        tipoSelecionado = "TECNICO";
      } else if (key == '3') {
        display.setCursor(60, 30);
        display.print("ADMINISTRADOR");
        display.display();
        tipoSelecionado = "ADMINISTRADOR"; 
      } else if (key == '#') {
        vTaskDelay(20);
        b = 5;
        if (opnav == true)
          screens();
        else
          telas();
        break;
      }
    }
    if (tipoSelecionado.length() > 0) {
      CadastrarCartao(tipoSelecionado); // Passa o tipo selecionado
    }
  }
  display.display();
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

// -----------------------------------------------------------------
// -----Manutenção-----
void manutencao(){
  display.clearDisplay();
  display.setCursor(20, 10);
  display.print("ESCOLHA A OPCAO:");
  display.setCursor(0, 20);
  display.print("1- MANU PREVENTIVA");
  display.setCursor(0, 30);
  display.print("2- MANU CORRETIVA");
  display.setCursor(80, 50);
  display.print("#-SAIR");
  display.display();
  vTaskDelay(80);

  while (opnav == true) {
    char key = kpd.getChar();
    vTaskDelay(90);
    esp_task_wdt_reset();
    if (key != 'N') {
      vTaskDelay(30);
      if (key == '1') {
        manup = 1;
        esp_task_wdt_reset();
        vTaskDelay(30);
        pref.putInt(prevpref, manup);
        status();
      } else if (key == '2') {
        manuc = 1;
        esp_task_wdt_reset();
        vTaskDelay(30);
        pref.putInt(correpref, manuc);
        status();
      } else if (key == '#') {
        manuc = 0;
        manup = 0;
        esp_task_wdt_reset();
        vTaskDelay(30);
        pref.putInt(correpref, manuc);
        pref.putInt(prevpref, manup);
        apx();
      }
    }
  }
}

// -----------------------------------------------------------------
// -----excluir-----
void excluir() {
  display.clearDisplay();
  display.setCursor(15, 30);
  display.print("APROXIME O CARTAO");
  display.display();
  while (1) {
    esp_task_wdt_reset();
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      snprintf(uid_buffer, sizeof(uid_buffer), "%02X%02X%02X%02X",
              rfid.uid.uidByte[0], rfid.uid.uidByte[1],
              rfid.uid.uidByte[2], rfid.uid.uidByte[3]);

      String cardToRemove = String(uid_buffer);
      bool cardFound = false;

      // Verifica e remove o cartão das listas correspondentes
      if (Operador.uid.indexOf(cardToRemove) != -1) {
        Operador.uid.replace(cardToRemove + ";", ""); // Remove o cartão com separador
        Operador.uid.replace(cardToRemove, ""); // Remove cartão sem separador no final
        pref.putString(Opepref, Operador.uid);  // Atualiza na EEPROM
        cardFound = true;
      } else if (Tecnico.uid.indexOf(cardToRemove) != -1) {
        Tecnico.uid.replace(cardToRemove + ";", "");
        Tecnico.uid.replace(cardToRemove, "");
        pref.putString(Tecpref, Tecnico.uid);
        cardFound = true;
      } else if (Adm.uid.indexOf(cardToRemove) != -1) {
        Adm.uid.replace(cardToRemove + ";", "");
        Adm.uid.replace(cardToRemove, "");
        pref.putString(Admpref, Adm.uid);
        cardFound = true;
      }

      display.clearDisplay();
      if (cardFound) {
        display.setCursor(45, 30);
        display.print("APAGADO");
        display.display();
      } else {
        display.setCursor(35, 30);
        display.print("CARTAO NAO");
        display.setCursor(35, 40);
        display.print("CADASTRADO");
        display.display();
      }
      esp_task_wdt_reset();
      vTaskDelay(500);
      
      if (opnav == true) {
        screens();
      } else {
        telas();
      }
      break; 
    }
  }
}

// -----------------------------------------------------------------
// -----formatar-----
void formatar(){
  display.clearDisplay();
  display.setCursor(35, 20);
  display.print("FORMARTAR?");
  display.setCursor(40, 40);
  display.print("1 - SIM");
  display.setCursor(40, 50);
  display.print("2 - NAO");
  display.display();
  while (1) {
    esp_task_wdt_reset();
    char key = kpd.getChar();
    vTaskDelay(90);
    if (key != 'N') {
      vTaskDelay(50);
      if (key == '1') {
        format();
        vTaskDelay(750);
        telas();
      } else if (key == '2') {
        telas();
      }
    }  
  }
}

// -----------------------------------------------------------------
// -----Input data-----
void input(){
  display.clearDisplay();
  display.setCursor(30, 10);
  display.print("HORIMETRO:");
  display.setCursor(80, 50);
  display.print("#-SAIR");
  display.display();
  while (opnav == true){
    char key = kpd.getChar();
    vTaskDelay(50);
    esp_task_wdt_reset();
    if (key != 'N'){
      vTaskDelay(20);
      if (key == '#'){
        screens();
        break;
      }else if(key == 'A' || key == 'B'){
        vTaskDelay(5);
      }else if (key == 'D'){
        hourcheck();
      }else if (key == 'C'){
        dell(2);
        break;
      }else if (key == '*'){
        erease(key, 2);
        break;
      }else{  
        tag(key, 2);
        break;
      }  
    }  
  }
}

// -----------------------------------------------------------------
// -----GeneralHourmeter check-----
void hourcheck(){
  display.setCursor(30, 10);
  display.print("O HORIMETRO ESTÁ CORRETO?");
  display.setCursor(30, 20);
  display.print(General.Hour);
  display.setCursor(30, 40);
  display.print("D-CONFIRMAR");
  display.setCursor(30, 50);
  display.print("C-CORRIGIR");
  display.display();
  while(opnav == true){
    esp_task_wdt_reset();
    char key = kpd.getChar();
    vTaskDelay(50);
    if(key == 'D'){  
      screens();
      break;
    }else if (key == 'C'){
      input();
      break;    
    }  
  }
}

// -----------------------------------------------------------------
// -----Questions-----
void vazamento(){
  display.clearDisplay();
  display.setCursor(35, 20);
  display.print("VAZAMENTO?");
  display.setCursor(40, 40);
  display.print("1 - SIM");
  display.setCursor(40, 50);
  display.print("0 - NAO");
  display.display();
  while (opnav == true) {
    esp_task_wdt_reset();
    char key = kpd.getChar();

    if (key != 'N') {
      vTaskDelay(50);
      if (key == '1') {
        Ans.Answer1 = 1;
        digitalWrite(OUT_PIN, LOW);
        garfos();
        break;
      } else if (key == '0') {
        Ans.Answer1  = 0;
        garfos();
        break;
      }
    }

  }
}

void garfos(){
  display.clearDisplay();
  display.setCursor(40, 20);
  display.print("GARFOS?");
  display.setCursor(40, 40);
  display.print("1 - SIM");
  display.setCursor(40, 50);
  display.print("0 - NAO");
  display.display();  
  while (opnav == true) {
    char key = kpd.getChar();
    esp_task_wdt_reset();
    if (key != 'N') {
      vTaskDelay(50);
      if (key == '1') {
        Ans.Answer2 = 1;
        emergencia();
        break;
      } else if (key == '0') {
        Ans.Answer2  = 0;
        digitalWrite(OUT_PIN, LOW);
        emergencia();
        break;
      }
    }
  }
}

void emergencia(){
  display.clearDisplay();
  display.setCursor(5, 20);
  display.print("BOTAO DE EMERGENCIA?");
  display.setCursor(40, 40);
  display.print("1 - SIM");
  display.setCursor(40, 50);
  display.print("0 - NAO");
  display.display();
  while (opnav == true) {
    char key = kpd.getChar();
    esp_task_wdt_reset();
    if (key != 'N') {
      vTaskDelay(50);
      if (key == '1') {
        Ans.Answer3 = 1;
        comando();
        break;
      } else if (key == '0') {
        Ans.Answer3 = 0;
        digitalWrite(OUT_PIN, LOW);
        comando();
        break;
      }
    }
  }
}

void comando(){
  display.clearDisplay();
  display.setCursor(30, 20);
  display.print("FRENTE E RE?");
  display.setCursor(40, 40);
  display.print("1 - SIM");
  display.setCursor(40, 50);
  display.print("0 - NAO");
  display.display();  
  while (opnav == true) {
    char key = kpd.getChar();
    esp_task_wdt_reset();
    if (key != 'N') {
      vTaskDelay(50);
      if (key == '1') {
        Ans.Answer4 = 1;
        bateria();
        break;
      } else if (key == '0') {
        Ans.Answer4 = 0;
        digitalWrite(OUT_PIN, LOW);
        bateria();
        break;
      }
    }
  }
}

void bateria(){
  display.clearDisplay();
  display.setCursor(5, 20);
  display.print("BATERIA,CABOS,CONEC?");
  display.setCursor(40, 40);
  display.print("1 - SIM");
  display.setCursor(40, 50);
  display.print("0 - NAO");
  display.display();
  while (opnav == true) {
    char key = kpd.getChar();
    esp_task_wdt_reset();
    if (key != 'N') {
      vTaskDelay(50);
      priority = 2;
      if (key == '1') {
        display.clearDisplay();
        display.setCursor(6, 1);
        display.print("CONCLUIDO");
        display.setCursor(2, 2);
        display.print("MAQUINA LIBERADA");
        Ans.Answer5 = 1;
        opnav = true;  
        telafinal();
        break;
      } else if (key == '0') {
        display.clearDisplay();
        display.setCursor(6, 1);
        display.print("CONCLUIDO");
        display.setCursor(2, 2);
        display.print("MAQUINA LIBERADA");
        Ans.Answer5 = 0;
        digitalWrite(OUT_PIN, LOW);
        opnav = true;  
        telafinal();
        break;
      }
    }
    display.display();
    manup = 0;
    manuc = 0;
    vTaskDelay(50);
    pref.putInt(correpref, manuc);
    pref.putInt(prevpref, manup);
  }
}

// -----------------------------------------------------------------
// -----final screen-----
void telafinal(){
  if (Ans.Answer1 == 1 ||Ans.Answer3 == 1||Ans.Answer5 == 1) {
    digitalWrite(LORA_CS, LOW); 
    digitalWrite(OUT_PIN, LOW);
    display.clearDisplay();
    display.setCursor(45, 10);
    display.print("IHM-V1.0");
    display.setCursor(40, 30); 
    display.print("GREENTECH");
    display.setCursor(15, 40);
    display.print("MAQUINA BLOQUEADA");
    display.setCursor(10, 50);
    display.print("BRIDGESTONE - TESTE");
    display.display();
  
    LoRa.beginPacket();
    LoRa.println(MID);
    LoRa.println("Maquina bloqueada");
    LoRa.endPacket();
  }else{
    digitalWrite(OUT_PIN, HIGH);
    display.clearDisplay();
    display.setCursor(45, 10);
    display.print("IHM-V1.0");
    display.setCursor(40, 30);
    display.print("GREENTECH");
    display.setCursor(10, 40);
    display.print("PRONTO PARA OPERAR");
    display.setCursor(10, 50);
    display.print("BRIDGESTONE - TESTE");
    display.display();
  } 

  digitalWrite(RFID_CS, LOW);  
  while (opnav == true) { 
    digitalWrite(RFID_CS, LOW); 
    esp_task_wdt_reset(); 
    char uid_buffer2[32];

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      snprintf(uid_buffer2, sizeof(uid_buffer2), "%02X%02X%02X%02X",
                        rfid.uid.uidByte[0], rfid.uid.uidByte[1],
                        rfid.uid.uidByte[2], rfid.uid.uidByte[3]);

      if (Adm.uid.indexOf(uid_buffer2) != -1){
        digitalWrite(OUT_PIN, HIGH);
        display.clearDisplay();
        display.setCursor(45, 10);
        display.print("IHM-V1.0");
        display.setCursor(40, 30);
        display.print("GREENTECH");
        display.setCursor(10, 40);
        display.print("PRONTO PARA OPERAR");
        display.setCursor(10, 50);
        display.print("BRIDGESTONE - TESTE");
        display.display();

        digitalWrite(LORA_CS, LOW); 
        LoRa.beginPacket(); 
        LoRa.println(MID);
        LoRa.println("Maquina desbloqueada pelo ADM");
        LoRa.endPacket();
      }
    }
    esp_task_wdt_reset(); 
  } 
}
// ------end code--------