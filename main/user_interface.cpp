#include "user_interface.h"
//#include "defines.h"


void screen_loading();
void screen_maintanence();
void screen_adm();
void screen_credentials();
void screen_operation();
void screen_blocked();
void screen_checklist_battery();
void screen_checklist_comando();
void screen_checklist_emergencia();
void screen_checklist_garfos();
void screen_checklist_vazamento();
void credentials_cb();
void maintanence_cb();
void adm_cb();
void checklist_cb();

#define N_SCREENS 10

typedef enum
{
ID_screen_loading                = 0,
ID_screen_maintanence            = 1,
ID_screen_adm                    = 2,
ID_screen_blocked                = 3,
ID_screen_operation              = 4,
ID_screen_credentials            = 5,
ID_screen_checklist_comando      = 6,
ID_screen_checklist_emergencia   = 7,
ID_screen_checklist_garfos       = 8,
ID_screen_checklist_vazamento    = 9,
} screen_id_t;

screen_definition_t screenDef[N_SCREENS] = {
  {ID_screen_loading              , screen_loading             , NULL          , "" },
  {ID_screen_maintanence          , screen_maintanence         , maintanence_cb, "1#" },
  {ID_screen_adm                  , screen_adm                 , adm_cb        , "1#" },
  {ID_screen_blocked              , screen_blocked             , NULL          , "" },
  {ID_screen_operation            , screen_operation           , NULL          , "" },
  {ID_screen_credentials          , screen_credentials         , credentials_cb, "#" },

  {ID_screen_checklist_comando    , screen_checklist_comando   , checklist_cb  , "01" },
  {ID_screen_checklist_emergencia , screen_checklist_emergencia, checklist_cb  , "01" },
  {ID_screen_checklist_garfos     , screen_checklist_garfos    , checklist_cb  , "01" },
  {ID_screen_checklist_vazamento  , screen_checklist_vazamento , checklist_cb  , "01" },
};


void set_screen(screen_id_t screen_id);

screen_id_t actual_screen = ID_screen_loading;
uint8_t update_screen=0;
int actual_login    =0;


static TwoWire *lcdtwi;
Adafruit_SSD1306 display;
I2CKeyPad keypad(KEYBOARD_ADDRESS);
char keypad_map[19] = "DCBA#9630852*741NF";


// Buffer and callback setup
#define KEYPAD_BUFFER_SIZE 8
#define KEYPAD_DETECTION_DELAY 3
char keypad_buffer[KEYPAD_BUFFER_SIZE] = "";
int keypadBufferIndex = 0;
int detection_delay = 0;



// -----------------------------------------------------------------

void screen_loading(){
  display.setCursor(10, 10); display.print("GREENTECH-V1.0");
  display.setCursor(15, 40); display.print("CARREGANDO");
}

void screen_maintanence(){
  display.setCursor(20, 10); display.print("ESCOLHA A OPCAO:");
  display.setCursor(0, 20);  display.print("1- BLOQUEAR/DESBLOQUEAR");
  display.setCursor(0, 30);  display.print("# - Sair");
}

void screen_adm(){
  display.setCursor(20, 10); display.print("ESCOLHA A OPCAO:");
  display.setCursor(0, 20);  display.print("1- BLOQUEAR/DESBLOQUEAR");
  display.setCursor(0, 30);  display.print("# - Sair");
}

void screen_credentials(){
  display.setCursor(0, 10);  display.print("LOGIN:");
  display.setCursor(0, 20);  display.print("__________");
  display.setCursor(0, 30);  display.print("SENHA:");
  display.setCursor(0, 40);  display.print("__________"); 
}

uint8_t hb_counter=0;
  void screen_operation(){
  display.setCursor(10, 10); display.print("GREENTECH V1.0 M"); display.print(121);
  //display.setCursor(15, 35); display.print("PRONTO PARA OPERAR");
  display.setCursor(30,20); display.print("BRIDGESTONE");


  display.setCursor(0, 40); 
  display.print("USER:"); display.print(actual_login); 

  display.setCursor(0, 50); 
  display.print("HB:"); display.print(hb_counter); 

  display.setCursor(50, 50); 
  display.print("E:"); display.print(0); 
/*
display.setCursor(80, 50);
display.print("V:"); display.print(operation_signals.vx); 


display.setCursor(20, 40); 
  display.print("a|");  display.print(operation_signals.imu0_a[0]); 
  display.print("|");   display.print(operation_signals.imu0_a[1]); 
  display.print("|");   display.print(operation_signals.imu0_a[2]); 

display.setCursor(20, 50);
  display.print(" g|"); display.print(operation_signals.imu0_g[0]); 
  display.print("|");   display.print(operation_signals.imu0_g[1]); 
  display.print("|");   display.print(operation_signals.imu0_g[2]); 
*/
  hb_counter++; 
}

void screen_blocked(){
  display.setCursor(10, 10); display.print("GREENTECH-V1.0");
  display.setCursor(15, 40); display.print("MAQUINA BLOQUEADA");
  display.setCursor(10, 50); display.print("CHAME ASSISTENCIA");
}

void screen_checklist_battery(){
  display.setCursor(5, 20);  display.print("BATERIA,CABOS,CONEC?");
  display.setCursor(40, 40); display.print("1 - SIM");
  display.setCursor(40, 50); display.print("0 - NAO");
}

void screen_checklist_comando(){
  display.setCursor(30, 20); display.print("FRENTE E RE?");
  display.setCursor(40, 40); display.print("1 - SIM");
  display.setCursor(40, 50); display.print("0 - NAO");
}

void screen_checklist_emergencia(){
  display.setCursor(5, 20);  display.print("BOTAO DE EMERGENCIA?");
  display.setCursor(40, 40); display.print("1 - SIM");
  display.setCursor(40, 50); display.print("0 - NAO");
}

void screen_checklist_garfos(){
  display.setCursor(40, 20); display.print("GARFOS?");
  display.setCursor(40, 40); display.print("1 - SIM");
  display.setCursor(40, 50); display.print("0 - NAO");
}

void screen_checklist_vazamento(){
  display.setCursor(35, 20); display.print("VAZAMENTO?");
  display.setCursor(40, 40); display.print("1 - SIM");
  display.setCursor(40, 50); display.print("0 - NAO");
}



//-------------------------------------------------------------------------------------------------------------------


bool check_credentials(int actual_login, int actual_password){
  if ((actual_login==1234) && (actual_password==1234)) return true;
  if ((actual_login==4321) && (actual_password==1234)) return true;

  return false;
}

bool should_ask_for_checklist(){
  return true;
}

uint8_t credentials_field = 0;
void credentials_cb(){
  Serial.printf("credentials_cb -> BUFFER: %s\n", keypad_buffer);
  
  if (credentials_field==0){
    credentials_field = 1;
    display.setCursor(0, 40);

    actual_login = atoi(keypad_buffer);
  }
  else{
    credentials_field = 0;

    int actual_password = atoi(keypad_buffer);
    if (check_credentials(actual_login, actual_password)){
      Serial.printf("credentials aproved");

      if (should_ask_for_checklist()){
        set_screen(ID_screen_checklist_comando);
      } 
      else{
        set_screen(ID_screen_operation);
        digitalWrite(KEY_OUT, HIGH);
      }                            


    }
    else{
      Serial.printf("credentials invalid");
      update_screen=1; //just refresh the screen
    }
  }
}

void adm_cb(){
  Serial.printf("adm_cb -> BUFFER: %s\n",keypad_buffer);
}

void maintanence_cb(){
  Serial.printf("maintanence_cb -> BUFFER: %s\n",keypad_buffer);
}


void checklist_cb(){
  Serial.printf("checklist_cb -> BUFFER: %s\n",keypad_buffer);
  uint8_t last_key = keypadBufferIndex-1;


  if (keypad_buffer[last_key] == '1'){
    if (actual_screen==N_SCREENS-1){
      set_screen(ID_screen_operation);
      digitalWrite(KEY_OUT, HIGH);
    }            
    else{
      set_screen(screen_id_t(int(actual_screen)+1)); 
    }                                             
  }  
  if (keypad_buffer[last_key] == '0')  set_screen(ID_screen_blocked);
  
}





//-------------------------------------------------------------------------------------------------------------------
          //digitalWrite(KEY_OUT, HIGH);


// Function to check if a key is in the array
inline bool isKeyInArray(char key, const char* array, size_t arraySize) {
  for (size_t i = 0; i < arraySize; i++) {
    if (array[i] == key) {
        return true;
    }
  }
  return false;
}

void read_i2c_keypad(){
  char keychar = keypad.getChar(); // Poll the keypad

  if (detection_delay!=0){
    detection_delay--;
    return;
  }

  if (keychar!='N') { // If a key is pressed
    detection_delay = KEYPAD_DETECTION_DELAY;

    Serial.printf("key pressed %c - BUFFER size: %d\n",keychar, keypadBufferIndex);

    if (keypadBufferIndex < KEYPAD_BUFFER_SIZE - 1) {
      keypad_buffer[keypadBufferIndex] = keychar; // Add the key to the buffer
      keypadBufferIndex++;

    } else {
      Serial.println("Buffer overflow!");
      keypad_buffer[0] = keychar; // Add the key to the buffer
      keypadBufferIndex=1;
    }

    char *trigger_keys = (screenDef[actual_screen].triggerKeys);

    Serial.print("trigger_keys ");
    Serial.println(trigger_keys);

    Serial.print("size of trigger_keys ");
    Serial.println(strlen(trigger_keys));

    if (actual_screen==ID_screen_credentials){
      if (keychar!='#'){
        if (credentials_field==0) display.print(keychar);
        else                      display.print("*");
        
        display.display();
      }else{
        keypadBufferIndex--;// remove the final #
      }
    }
    if (isKeyInArray(keychar, trigger_keys, strlen(trigger_keys) ) ) {
      // Call the callback and clear the buffer
      keypad_buffer[keypadBufferIndex] = '\0'; // Null-terminate the buffer
      
      screenDef[actual_screen].onKeyDetectedCB();
      keypadBufferIndex = 0;           // Reset the buffer
    }
  } 
}


void set_screen(screen_id_t screen_id){
actual_screen = screen_id;
update_screen = 1;
}


void user_interface_ask_credentials(){
set_screen(ID_screen_credentials);
//set_screen(ID_screen_operation);
}

void run_update_screen(){
if (actual_screen==ID_screen_operation){
  update_screen =1;
}
}

void user_interface_run(){
if (update_screen==1){
  display.clearDisplay();
  screenDef[actual_screen].screenDisplay();
  display.display();
    
  if (actual_screen==ID_screen_credentials) display.setCursor(0, 20);
  update_screen = 0;
}

read_i2c_keypad();
}


//--------------------------------------------------------------------------------------------------------------------------------------------------------------

void LCDWriteChars(int x,int y, char* text,int len){
display.setCursor(x, y); 
  for(int16_t i=0; i<len; i++) {
  display.write(text[i]);
}
}


void LCDWriteChars(char* text,int len){
display.clearDisplay();

display.setTextSize(1);      // Normal 1:1 pixel scale
display.setTextColor(SSD1306_WHITE); // Draw white text
display.setCursor(0, 0);     // Start at top-left corner
display.cp437(true);         // Use full 256 char 'Code Page 437' font

// Not all the characters will fit on the display. This is normal.
// Library will draw what it can and the rest will be clipped.
for(int16_t i=0; i<len; i++) {
  display.write(text[i]);
}

display.display();
}


void LCDconfigureTWI(TwoWire *twi){
lcdtwi = twi;

display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, lcdtwi, OLED_RESET);
}


bool setup_lcd() {  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    return -1;
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

  return 1;
}

void keypad_setup(TwoWire *twi){
  keypad.begin();
  keypad.setKeyPadMode(I2C_KEYPAD_4x4);
  keypad.loadKeyMap(keypad_map);
} 


