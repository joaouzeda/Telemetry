/*-----------------------------------------------------------------

  Telemetry V0.5.1 main.cpp
     
  INA226
  MFRC522 
  LCD 16x2 display

  Compiler: VsCode 1.88.1
  MCU: ESP32
  Board: Dev module 28 pins

  Author: João  G. Uzêda & Victor Hugo
  date: 2024, May

-----------------------------------------------------------------*/

// ----------------------------------------------------------------- 
// ---Libraries---
#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "LiquidCrystal_I2C.h"
#include "freertos/task.h"
#include "PubSubClient.h"
#include "esp_task_wdt.h"
#include "Preferences.h"
#include "Keypad_I2C.h"
#include "Password.h"
#include "rtc_wdt.h"
#include "Arduino.h"
#include "MFRC522.h"
#include "INA226.h"
#include "Keypad.h"
#include "WiFi.h"
#include "Wire.h"
#include "SPI.h"
// -----------------------------------------------------------------  

// ----------------------------------------------------------------- 
// ---defines---
#define SHUNT_RESISTENCE 0.75
const byte ROWS = 4;
const byte COLS = 4;
// ----------------------------------------------------------------- 

// -----------------------------------------------------------------  
// ----Keypad---- 
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
// -----------------------------------------------------------------  

// -----------------------------------------------------------------  
//----I²C Adresses------
#define LCDADRESS 0x27 
#define INAADRESS 0x40 
#define KeyAdress 0x21
// -----------------------------------------------------------------  

// -----------------------------------------------------------------  
//----pins------
#define   SS        22
#define   RST       5
#define   SCK       14
#define   MISO      2
#define   MOSI      15
#define   SDA       1
#define   SCL       3 
byte rowPins[ROWS] = { 0, 1, 2, 3 };
byte colPins[COLS] = { 4, 5, 6, 7 };
// -----------------------------------------------------------------  

// ----------------------------------------------------------------- 
// ---connection infos--
const char *ssid    =    "Greentech_Administrativo";             
const char *pass    =    "Gr3enTech@2O24*";   
const char *mqtt    =    "192.168.30.130";      // rasp nhoqui
//const char *mqtt    =    "192.168.30.212";    // rasp eng
const char *user    =    "greentech";                           
const char *passwd  =    "Greentech@01";                       
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
const char *Topic_user    =    "proto/sim/Usuario";  
const char *topic_V       =    "proto/sim/checklist/P1:";                   
const char *topic_G       =    "proto/sim/checklist/P2:";                      
const char *topic_E       =    "proto/sim/checklist/P3:";               
const char *topic_F       =    "proto/sim/checklist/P4";              
const char *topic_B       =    "proto/sim/checklist/P5:";  
const char *topic_TEC     =    "proto/sim/manutenção";
const char *topic_CAD     =    "sinal/sim/Cadastro";
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// -----Functions----
void recon();
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
//Password password = Password("2552"); 
LiquidCrystal_I2C lcd(LCDADRESS, 16, 2);
//Keypad_I2C kpd(makeKeymap(keys), rowPins, colPins,
//                     ROWS, COLS, KeyAdress, PCF8574);  
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// ----Variables----
float geralA;
float Volt;
float ABombH;
float ATrac;
int hourmeter;
int hourmeterT;
int hourmeterB;
int minute;
int minuteT;
int minuteB;
int sec;
int secT;
int secB;
int opa = 0;
byte a = 7;
byte b = 5;
byte maxtaglen = 6; //currentPasswordLength
byte maxpasslen = 5;  //maxPasswordLength
byte currenttaglen = 0; //PassLenghtMax
byte currentpasslen = 0;  //PassLenghtAtual
bool passvalue = true;

// -----------------------------------------------------------------

// -----------------------------------------------------------------
// --Preferences Key---
const char *minpref   =   "min";
const char *mintrac   =   "trac";
const char *minbomb   =   "minbomb";
const char *hourpref  =   "hour";
const char *hourtrac  =   "hourtrac";
const char *hourbomb  =   "hourbomb";
const char *prevpref  =   "Manupreve";
const char *correpref =   "Manucorre";
const char *cadaspref =   "Cadastro";
const char *listapref =   "Cadastro Cartoes";
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// UIDS
String OpeCard  =   "6379CF9A";  
String AdmCard  =   "29471BC2";  
String TecCard  =   "D2229A1B";  
String PesCard  =   "B2B4BF1B";
String UIDLists =   "";
String TAGLists =   "";
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// buffers 
char uid_buffer[32];
bool opnav;
bool engnav;
int  manup; //preve
int  manuc; //corre
// -----------------------------------------------------------------

extern "C" void app_main(){
  initArduino();
  Wire.begin(SDA, SCL);
  SPI.begin(SCK, MISO, MOSI, RST);
  //kpd.begin(makeKeymap(keys));
  rfid.PCD_Init();
  pref.begin("GT", false);
  WiFi.begin(ssid, pass); 
  lcd.init();
  lcd.backlight();
  ina226_setup();

  lcd.home();
  lcd.print("INICIALIZANDO");
  vTaskDelay(500);
  
  if(WiFi.status() != WL_CONNECTED)
    WiFi.reconnect();                                                                                               

  client.setServer(mqtt, port);           
  if (!client.connected()){                            
    client.connect("TestClient", user, passwd);    
    vTaskDelay(500);                                         
  } 

  minuteB     = pref.getInt(minbomb, minuteB);
  minute      = pref.getInt(minpref, minute);
  hourmeter   = pref.getInt(hourpref, hourmeter); 
  hourmeterT  = pref.getInt(hourtrac, hourmeterT);
  hourmeterB  = pref.getInt(hourbomb, hourmeterB);

  manup = pref.getInt(prevpref, manup);
  manuc = pref.getInt(correpref, manuc);

  xTaskCreatePinnedToCore(xTaskTelemetry, // function name
                          "Telemetry",    // task name
                          8000,           // stack size in word
                          NULL,           // input parameter
                          1,              // priority
                          NULL,           // task handle
                          1);                // core

  xTaskCreatePinnedToCore(xTaskNav,
                          "Navegation",
                          8000,
                          NULL,
                          2,
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
  char CATrac[30];
  char CABombH[30];
  char Justhour[30];
  char FullHour[30];
  char JusthourT[30];
  char JusthourB[30];
  char Amp_buffer[30];
  char Volt_buffer[30];
  char hourmeter1[30];

  esp_task_wdt_add(NULL);        //  enable watchdog     
  while(1){  
    rtc_wdt_feed();    //  feed watchdog 

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

    if (ABombH >= 13){  // --------Hidraulic bomb hourmeter----------------------
      secB++;
      if (secB >= 60){        
        secB-=60;
        minuteB++;
        if (minuteB >= 1) // normaly minuteB == 10 
          pref.putInt(minbomb, minuteB);
        else if (minuteB == 20)
          pref.putInt(minbomb, minuteB);       
      }
      if (minuteB >= 60){
        minuteB-=60;
        hourmeterB++;
        pref.remove(minbomb);
        pref.putInt(hourbomb, hourmeterB);
      }
    } // --------------------------------------------------------------------------

    if (geralA >= 13){  // ---------Trasion engine hourmeter----------------------
      secT++;
      if (secT >= 60){        
        secT-=60;
        minuteT++;
        if (minuteT == 20)
          pref.putInt(mintrac, minuteT);
        else if (minuteT == 40)
          pref.putInt(mintrac, minuteT);
      }
      if (minuteT >= 60){
        minuteT-=60;
        hourmeterT++;
        pref.remove(mintrac);
        pref.putInt(hourtrac, hourmeterT);
      }
    } // -----------------------------------------------------------------------
    
    sprintf(CABombH, "{\"Corrente Bomba\":%.02f}", ABombH);
    sprintf(Ageral, "{\"Corrente geral\":%.02f}", geralA);
    sprintf(CATrac, "{\"Corrente tracao\":%.02f}", ATrac);
    sprintf(CVolt, "{\"Tensao geral\":%.02f}", Volt);
    sprintf(FullHour, "{\"hourmeter\": %02d:%02d:%02d}",
                                hourmeter, minute, sec);
    sprintf(hourmeter1, "%02d:%02d:%02d", hourmeterB, 
                                      minuteB, secB);
    sprintf(Justhour, "{\"JustHour\":%d}", hourmeter);
    sprintf(JusthourB, "{\"TracHour\":%d}", hourmeterB);
    sprintf(JusthourT, "{\"HbombHour\":%d}", hourmeterT);
    sprintf(Amp_buffer, "%.02f", geralA);
    sprintf(Volt_buffer, "%.02f", Volt);
    
    client.publish(topic_A,     Ageral);
    client.publish(topic_A2,    CABombH);
    client.publish(topic_A3,    CATrac);
    client.publish(topic_T,     CVolt);
    client.publish(topic_HBomb, JusthourB);
    client.publish(topic_Htrac, JusthourT);
    client.publish(topic_FHour, FullHour);
    client.publish(topic_JHour, Justhour);

    lcd.setCursor(0, 0);
    lcd.print("HB:");
    lcd.setCursor(4, 0);
    lcd.print(hourmeter1);

    lcd.setCursor(0, 1);
    lcd.print("A:");
    lcd.setCursor(3, 1);
    lcd.print(geralA, 3);

    lcd.setCursor(9, 1);
    lcd.print("V:");
    lcd.setCursor(11, 1);
    lcd.print(Volt);

    esp_task_wdt_reset();        // reset watchdog if dont return any error

    client.loop();
    vTaskDelay(1000/ portTICK_PERIOD_MS);
  }
} 
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// -----Navgation-----
void xTaskNav(void *pvParameters){
  esp_task_wdt_add(NULL);      //  enable watchdog     
  while(1){
    rtc_wdt_feed();                  //  feed watchdog 
   
    esp_task_wdt_reset();            // reset watchdog if dont return any error
    vTaskDelay(1000/ portTICK_PERIOD_MS);
  }
}

/*-------------------------------------------------------------------
--------------------------------FUNCTIONS----------------------------
-------------------------------------------------------------------*/

// -----------------------------------------------------------------
// -----Reconection-----
void recon(){
  WiFi.disconnect();        
  vTaskDelay(1000);         
  WiFi.begin(ssid, pass);   
  vTaskDelay(1000);         
    
  vTaskDelay(500);
                
  client.connect("TestClient", user, passwd);     
  vTaskDelay(5000);                                  
        
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








/*-----------------------------------------------------------------

  Telemetry V0.3 main.cpp

  MQtt
  Flash memory
  Hourmeter counter      
  INA + shunt system
  Display OLED
  LCD 16x2 display

  Compiler: VsCode 1.88.1
  MCU: ESP32 DEV MODULE

  Author: João  G. Uzêda && Victor Martins
  date: 2024, May

-----------------------------------------------------------------*/

// ----------------------------------------------------------------- 
// ---Libraries---
#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "LiquidCrystal_I2C.h"
#include "freertos/task.h"
#include "PubSubClient.h"
#include "esp_task_wdt.h"
#include "Preferences.h"
#include "Password.h"
#include "rtc_wdt.h"
#include "Arduino.h"
#include "MFRC522.h"
#include "WiFi.h"
#include "Wire.h"
#include "SPI.h"
#include "Keypad.h"
#include "Keypad_I2C.h"
// ---------------------------------------------------------------

// ---------------------------------------------------------------
#define LCDADRESS 0x27
#define I2CADDR 0x21 
// ----------------------------------------------------------------

// ---------------------------------------------------------------- 
//----pins----
#define SS_PIN 1
#define RST_PIN 3
// -----------------------------------------------------------------  

// ----------------------------------------------------------------- 
// ---connection infos--
const char *ssid    =    "Greentech_Administrativo";              // wifi name      
const char *pass    =    "Gr3enTech@2O24*";                       // wifi password
const char *mqtt    =    "192.168.30.212";                        // broker IP
const char *user    =    "greentech";                             // broker user      
const char *passwd  =    "Greentech@01";                          // broker password     
int port  =  1883;
// -----------------------------------------------------------------

// -----------------------------------------------------------------  
// -----Topics----- 
const char *topic_user    =    "sinal/sim/rfid";
const char *topic_CAD     =    "sinal/sim/Cadastro";
const char *topic_TEC     =    "sinal/sim/manutenção";  
const char *topic_V       =    "sinal/sim/checklist/P1:";        // broker topic Vazamento           
const char *topic_G       =    "sinal/sim/checklist/P2:";        // broker topic Garfo              
const char *topic_E       =    "sinal/sim/checklist/P3:";        // broker topic Emergencia       
const char *topic_F       =    "sinal/sim/checklist/P4:";        // broker topic Frente e Ré     
const char *topic_B       =    "sinal/sim/checklist/P5:";        // broker topic Bateria, cabos, conectores
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// --Preferences Key---
const char *prevpref = "Manupreve";
const char *correpref = "Manucorre";
const char *cadaspref = "Cadastro";
const char *listapref = "Cadastro Cartoes";
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// ----Variables----
char uid_buffer[32];
char CAD[32];
bool acertou;
bool navcheck;
bool value = true;
int preve;
int corre;
int marcadorNovoCartao = 0;
byte a = 7;
byte b = 5;
byte maxPasswordLength = 5;
byte currentPasswordLength = 0;
byte PassLenghtMax = 6;
byte PassLenghtAtual = 0;
String cartoesCadastrados = "";  // Salvar os cartões cadastrados
// -----------------------------------------------------------------

// -----------------------------------------------------------------
// ----Definition keypad----  
const byte ROWS = 4;
const byte COLS = 4;
// -----------------------------------------------------------------

// -----------------------------------------------------------------  
//----pins Keypad------
byte rowPins[ROWS] = { 0, 1, 2, 3 };
byte colPins[COLS] = { 4, 5, 6, 7 };
// ----------------------------------------------------------------- 

// -----------------------------------------------------------------
// ----Keypad---- 
char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};
// ----------------------------------------------------------------- 

// -----------------------------------------------------------------
// UIDS
String OpeCard  =  "6379CF9A";  
String AdmCard  =  "29471BC2";  
String TecCard  =  "D2229A1B";  
String PesCard  =  "B2B4BF1B";
String card     =  "E6A1191E";
String battery2 =  "F4E0A08D";
// -----------------------------------------------------------------
char pss[]  = "2552";
// -----------------------------------------------------------------
// -----Objects----
Preferences prefs; 
WiFiClient ESP32Clientv; 
MFRC522 rfid(SS, RST_PIN); 
PubSubClient client(ESP32Clientv); 
Password password = Password(pss);  
LiquidCrystal_I2C lcd(0x27, 20, 4);
Keypad_I2C kpd(makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR, PCF8574);  
// ----------------------------------------------------------------- 

// -----------------------------------------------------------------
// ----Functions---- 
void recon();
void eng();
void processNumberKey(char key);
void aprovadoPass();
void resetPassword();
void telas();
void cadastrar();
void CadastrarCartao();
void formatar();
void excluir();
void dell();
void tag(char key);
void apx();
void vazamento();
void garfos();
void emergencia();
void comando();
void bateria();
void manutencao();
void status();
void screens();
void telafinal();
// ----------------------------------------------------------------- 

// -----------------------------------------------------------------
// ---- ---- 
extern "C" void app_main(){
  Wire.begin();
  kpd.begin(makeKeymap(keys));
  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  SPI.begin();
  rfid.PCD_Init();
  lcd.init();
  lcd.backlight();
  prefs.begin("manu");

  lcd.clear();
  lcd.setCursor(3, 2);
  lcd.print("INICIALIZANDO");
  vTaskDelay(900);

  preve = prefs.getInt(prevpref, preve);
  corre = prefs.getInt(correpref, corre);
   
  // connect to wifi
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(1000);
  }

  // connect to broker
  client.setServer(mqtt, port);
  while (!client.connected()) {
    client.connect("ESP32Clientv", user, passwd);
    vTaskDelay(500);
  }
  
  while(1){

    client.loop();

  char menu = kpd.getKey();

  if (preve == 1) {
    status();
  } else if (corre == 1) {
    status();
  } else {
    if (menu != NO_KEY) {
      vTaskDelay(80);
      if (menu == '0') {
        navcheck = false;  // Para tela de Engenharia
        eng();
      }
    } else {
      navcheck = true;  // Para tela de Operação
      apx();
    }
  }
    vTaskDelay(1000);
  }    
}
//======================================================================TELAS ENGENHARIA======================================================================//
void eng() {

  lcd.clear();
  lcd.setCursor(5, 1);
  lcd.print("PASSWORD:");
  lcd.setCursor(14, 3);
  lcd.print("#-SAIR");

  while (1) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      vTaskDelay(20);
      if (key == 'C') {
        resetPassword();
      } else if (key == '#') {
        apx();
      } else if (key == 'D') {
        if (value == true) {
          aprovadoPass();
          value = false;
        }
      } else {
        processNumberKey(key);
      }
    }
  }
}

void processNumberKey(char key) {

  lcd.setCursor(a, 2);
  lcd.print("*");
  a++;
  if (a == 11) {
    a = 4;  // Tamanho da senha com 4 digitos "2552"
  }
  currentPasswordLength++;
  password.append(key);

  if (currentPasswordLength == maxPasswordLength) {
    aprovadoPass();
  }
}

void aprovadoPass() {

  currentPasswordLength = 0;

  if (password.evaluate()) {
    lcd.clear();
    lcd.setCursor(7, 2);
    lcd.print("VALIDO");
    vTaskDelay(1000);
    a = 7;
    acertou = true;  // mostra que acertou, apaga a mensagem anterior e segue para a tela screens ou  telas
    if (navcheck == true) {
      screens();
    } else {
      telas();
    }
  } else {
    lcd.clear();
    lcd.setCursor(6, 1);
    lcd.print("INVALIDO");
    lcd.setCursor(2, 2);
    lcd.print("TENTE NOVAMENTE");
    vTaskDelay(1000);
    a = 7;
    acertou = false;  // mostra que acertou, apaga a mensagem anterior e  volta para a tela de Password e colocar a senha correta
  }
  resetPassword();
}

void resetPassword() {

  password.reset();
  currentPasswordLength = 0;
  lcd.clear();
  a = 7;

  if (acertou != true) {
    eng();  // Mostra que errou a senha, apaga a senha sem sumir a mensagem "Password"
  }
}

void telas() {

  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("ESCOLHA A OPCAO:");
  lcd.setCursor(0, 1);
  lcd.print("1- FORMATAR");
  lcd.setCursor(0, 2);
  lcd.print("2- EXCLUIR");
  lcd.setCursor(0, 3);
  lcd.print("#- SAIR");

  while (1) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      vTaskDelay(20);
      if (key == '1') {
        formatar();
      } else if (key == '2') {
        excluir();
      } else if (key == '#') {
        navcheck = true;  // True, após apertar "#" ele habilita a opção para bater os RFIDs novamente para seguir com outras opções.
        apx();
      }
    }
  }
}

void cadastrar() {

  lcd.clear();
  lcd.setCursor(1, 1);
  lcd.print("TAG:");
  lcd.setCursor(1, 2);
  lcd.print("RFID:");
  lcd.setCursor(14, 3);
  lcd.print("#-SAIR");
  acertou = false;

  while (navcheck == true) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      vTaskDelay(20);
      if (key == 'C') {
        dell();
      } else if (key == '#') {
        b = 5;
        screens();
      } else {
        tag(key);
      }
    }
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      snprintf(CAD, sizeof(CAD), "%02X%02X%02X%02X",
               rfid.uid.uidByte[0], rfid.uid.uidByte[1],
               rfid.uid.uidByte[2], rfid.uid.uidByte[3]);

      b = 5;  // Sempre que bater o RFID a proxima TAG volta ser digitada na posição 5
      client.publish(topic_CAD, "");
      lcd.setCursor(6, 2);
      lcd.print(CAD);
      vTaskDelay(1000);
      cadastrar();
    }
  }
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void CadastrarCartao() {

  String conteudo = "";

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    snprintf(CAD, sizeof(CAD), "%02X%02X%02X%02X",
             rfid.uid.uidByte[0], rfid.uid.uidByte[1],
             rfid.uid.uidByte[2], rfid.uid.uidByte[3]);
    conteudo.concat(CAD);  // Juntando os valores do CAD no conteudo
    conteudo = conteudo + ";";
    cartoesCadastrados = cartoesCadastrados + conteudo;


    prefs.putString(listapref, cartoesCadastrados);
  }t char*
}





void dell() {

  password.reset();
  currentPasswordLength = 0;
  lcd.clear();
  b = 5;

  if (acertou != true) {
    cadastrar();
  }
}

void tag(char key) {

  lcd.setCursor(b, 1);
  lcd.print(key);
  b++;

  if (b == 11) {
    b = 5;  // Tamanho da TAG com 6 digitos "333..."
  }
  PassLenghtAtual++;
  password.append(key);
}
//=======================================================================TELAS OPERAÇÃO========================================================================//
void apx() {

  lcd.clear();
  lcd.setCursor(2, 2);
  lcd.print("APROXIMAR CARTAO");

  while (navcheck == true) {

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      snprintf(uid_buffer, sizeof(uid_buffer), "%02X%02X%02X%02X",
               rfid.uid.uidByte[0], rfid.uid.uidByte[1],
               rfid.uid.uidByte[2], rfid.uid.uidByte[3]);

      client.publish(topic_user, uid_buffer);

      if (strcmp(uid_buffer, OpeCard.c_str()) == 0) {
        lcd.clear();
        lcd.setCursor(6, 1);
        lcd.print("OPERADOR");
        lcd.setCursor(4, 2);
        lcd.print("IDENTIFICADO");
        vTaskDelay(1000);
        vazamento();
      } else if (strcmp(uid_buffer, TecCard.c_str()) == 0) {
        lcd.clear();
        lcd.setCursor(6, 1);
        lcd.print("TECNICO");
        lcd.setCursor(4, 2);
        lcd.print("IDENTIFICADO");
        vTaskDelay(1000);
        manutencao();
      } else if (strcmp(uid_buffer, AdmCard.c_str()) == 0) {
        lcd.clear();
        lcd.setCursor(3, 1);
        lcd.print("ADMINISTRADOR");
        lcd.setCursor(4, 2);
        lcd.print("IDENTIFICADO");
        vTaskDelay(1000);
        eng();
      }
    }
  }
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void vazamento() {

  lcd.clear();
  lcd.setCursor(5, 0);
  lcd.print("VAZAMENTO?");
  lcd.setCursor(0, 2);
  lcd.print("1 - SIM");
  lcd.setCursor(0, 3);
  lcd.print("2 - NAO");

  while (navcheck == true) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      vTaskDelay(20);
      if (key == '1') {
        client.publish(topic_V, "True");
        garfos();
      } else if (key == '2') {
        client.publish(topic_V, "False");
        garfos();
      }
    }
    if (WiFi.status() != WL_CONNECTED  || !client.connected()) 
        recon();
  }
}

void garfos() {

  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("GARFOS?");
  lcd.setCursor(0, 2);
  lcd.print("1 - SIM");
  lcd.setCursor(0, 3);
  lcd.print("2 - NAO");

  while (navcheck == true) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      vTaskDelay(20);
      if (key == '1') {
        client.publish(topic_G, "True");
        emergencia();
      } else if (key == '2') {
        client.publish(topic_G, "False");
        emergencia();
      }
    }
  }
}

void emergencia() {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BOTAO DE EMERGENCIA?");
  lcd.setCursor(0, 2);
  lcd.print("1 - SIM");
  lcd.setCursor(0, 3);
  lcd.print("2 - NAO");

  while (navcheck == true) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      vTaskDelay(20);
      if (key == '1') {
        client.publish(topic_E, "True");
        comando();
      } else if (key == '2') {
        client.publish(topic_E, "False");
        comando();
      }
    }
  }
}

void comando() {

  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("FRENTE E RE?");
  lcd.setCursor(0, 2);
  lcd.print("1 - SIM");
  lcd.setCursor(0, 3);
  lcd.print("2 - NAO");

  while (navcheck == true) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      vTaskDelay(20);
      if (key == '1') {
        client.publish(topic_F, "True");
        bateria();
      } else if (key == '2') {
        client.publish(topic_F, "False");
        bateria();
      }
    }
  }
}

void bateria() {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BATERIA,CABOS,CONEC?");
  lcd.setCursor(0, 2);
  lcd.print("1 - SIM");
  lcd.setCursor(0, 3);
  lcd.print("2 - NAO");

  while (navcheck == true) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      vTaskDelay(20);
      if (key == '1') {
        client.publish(topic_B, "True");
        lcd.clear();
        lcd.setCursor(6, 1);
        lcd.print("CONCLUIDO");
        lcd.setCursor(2, 2);
        lcd.print("MAQUINA LIBERADA");
        vTaskDelay(1000);
        navcheck = true;  // "Mudar futuramente de 'true' para 'false' quando for para tela de funcionando", deixando true ele não zera todos para false após repetir o checklist novamente
        telafinal();
      } else if (key == '2') {
        client.publish(topic_B, "False");
        lcd.clear();
        lcd.setCursor(6, 1);
        lcd.print("CONCLUIDO");
        lcd.setCursor(2, 2);
        lcd.print("MAQUINA LIBERADA");
        vTaskDelay(1000);
        navcheck = true;  // "Mudar futuramente de 'true' para 'false' quando for para tela de funcionando", deixando true ele não zera todos para false após repetir o checklist novamente
        telafinal();
      }
    }
    corre = 0;
    preve = 0;
    vTaskDelay(50);
    prefs.putInt(correpref, corre);
    prefs.putInt(prevpref, preve);
  }
}

void manutencao() {

  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("ESCOLHA A OPCAO:");
  lcd.setCursor(0, 1);
  lcd.print("1- MANU PREVENTIVA");
  lcd.setCursor(0, 2);
  lcd.print("2- MANU CORRETIVA");
  lcd.setCursor(14, 3);
  lcd.print("#-SAIR");
  vTaskDelay(80);

  while (navcheck == true) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      vTaskDelay(20);
      if (key == '1') {
        preve = 1;
        vTaskDelay(30);
        prefs.putInt(prevpref, preve);
        status();
      } else if (key == '2') {
        corre = 1;
        vTaskDelay(30);
        prefs.putInt(correpref, corre);
        status();
      } else if (key == '#') {
        corre = 0;
        preve = 0;
        vTaskDelay(30);
        prefs.putInt(correpref, corre);
        prefs.putInt(prevpref, preve);
        apx();
      }
    }
  }
}

void status() {

  lcd.clear();

  while (1) {

    lcd.setCursor(6, 0);
    lcd.print("MAQUINA");
    lcd.setCursor(9, 1);
    lcd.print("EM");
    lcd.setCursor(5, 2);
    lcd.print("MANUTENCAO");

    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      char SAIR[32];
      snprintf(SAIR, sizeof(SAIR), "%02X%02X%02X%02X",
               rfid.uid.uidByte[0], rfid.uid.uidByte[1],
               rfid.uid.uidByte[2], rfid.uid.uidByte[3]);
      client.publish(topic_user, SAIR);

      if (strcmp(SAIR, TecCard.c_str()) == 0) {
        corre = 0;
        preve = 0;
        prefs.putInt(correpref, corre);
        prefs.putInt(prevpref, preve);
        client.publish(topic_TEC, " ");
        navcheck = true;
        vTaskDelay(80);
        apx();
      }
    } else if (!rfid.PICC_IsNewCardPresent() && !rfid.PICC_ReadCardSerial()) {

      if (preve == 1) {
        client.publish(topic_TEC, "Preventiva");
        lcd.setCursor(5, 3);
        lcd.print("PREVENTIVA");
        vTaskDelay(40);
      } else if (corre == 1) {
        client.publish(topic_TEC, "Corretiva");
        lcd.setCursor(5, 3);
        lcd.print("CORRETIVA");
        vTaskDelay(40);
      }
    }
  }
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void screens() {

  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("ESCOLHA A OPCAO:");
  lcd.setCursor(0, 1);
  lcd.print("1- CADASTRAR");
  lcd.setCursor(0, 2);
  lcd.print("2- EXCLUIR");
  lcd.setCursor(0, 3);
  lcd.print("#- SAIR");

  while (navcheck == true) {

    char key = kpd.getKey();

    if (key != NO_KEY) {
      vTaskDelay(20);
      if (key == '1') {
        navcheck = true;
        cadastrar();
      } else if (key == '2') {
        navcheck = true;
        excluir();
      } else if (key == '#') {
        apx();
      }
    }
  }
}

void telafinal() {

  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("NMS-V0.3");
  lcd.setCursor(5, 1);
  lcd.print("GREENTECH");
  lcd.setCursor(3, 2);
  lcd.print("OPERANDO");
  lcd.setCursor(0, 3);
  lcd.print("SHOWROOM-SP");
  vTaskDelay(1500);
 
}
/*---------------------------------------
----------------FUNCTIONS----------------
---------------------------------------*/
void recon(){
  while (WiFi.status() != WL_CONNECTED){
    WiFi.disconnect();        
    vTaskDelay(1000);        
    WiFi.begin(ssid, pass);  
    vTaskDelay(1000);        
  }        
  vTaskDelay(500);
  while (!client.connected()){                        
    client.connect("ESP32Client", user, passwd);    
    vTaskDelay(5000);                                  
  }                    
}




