// THE GOAL for the Version 3, is to optimaze the logic.
// - Fix some errors with the Rage Shield
// - Take in account, BUSY DFPlayer and BUSY NFCReader
// - Replace Delay for millis()
// - Add Timeouts

#define DEBUG 1 // set out to 0 to remove serial Prints

#if DEBUG
  #define D_print(...) Serial.print(__VA_ARGS__)
  #define D_println(...) Serial.println(__VA_ARGS__)
#else 
  #define D_print(...) 
  #define D_println(...) 
#endif

//BASIC
#include "Arduino.h"
#include "SoftwareSerial.h"

//PINOUT 
#define rxPin 11
#define txPin 10
#define BTN_PIN 5
#define BUSY_PIN 12
// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN 2


//DFPLAYER
#include <DFPlayerMini_Fast.h>
#define VOLUME_LEVEL 16
#define MP3_SOUNDS_FOLDER 10 //Init sound
#define MP3_EFFECTS_FOLDER 01 //Shield Bash Sound
#define MP3_ALTERN_FOLDER 02 //Alternative Sounds
SoftwareSerial mySoftwareSerial(rxPin, txPin); // RX, TX
DFPlayerMini_Fast myDFPlayer;
boolean has_media = true;
boolean isPlaying = false;
int num_tracks_in_folder = 0;
int actual_folder = 1;
int actual_track_n = 0;

// NFC libs
//including the library for I2C communication
#include <Wire.h>
//including the library for I2C communication with the PN532 module, library for the PN532 module and the library for the NFC
#include <PN532_I2C.h>
#include <Adafruit_PN532.h>
#include <PN532_HSU.h>
#include <NfcAdapter.h>
String last_card_UID = "";
boolean new_card = false;
String type = "Not recognized";
boolean waiting = false;

//constructors for the library for I2C communication with the module and for the NFC
PN532_I2C pn532_i2c(Wire);
PN532 nfc(pn532_i2c);

String tagId = "None", dispTag = "None";
byte nuidPICC[4];

// NEOPIXEL
#include <Adafruit_NeoPixel.h>
// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 10
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
#define BRIGHTNESS 240
int red = 0;
int green = 0;
int blue = 0;
char stripColor = 'G';

// declare structure for rgb variable
struct rgbColor {
  byte r;  // red value 0 to 4095
  byte g;  // green value
  byte b;  // blue value
};
rgbColor LEDS_COLOR = { 0, 255, 0};
//Multiple Colors
rgbColor GREEN = { 0, 255, 0};
rgbColor RED = { 255, 0, 0};
rgbColor YELLOW = { 251, 245, 10};
//
rgbColor NextCOLOR = { 0, 0, 0};

//Initiation steps
int init_step = 0;
boolean initiated = false;

//MODE - defined by Button action
int lastStatus = 0;
int mode = 0; // Shield Modes - RageMode / NormalMode
int power_mode = 0; //Power Modes - On / Off - SleepMode ( low consumption )

#define SEC 1000
#define POWER_ACTION_TIME 1100
#define SWITCH_ACTION_TIME 1000
auto currentLoopTime = 0;

const unsigned long debounceTime = 50;  // Tiempo para el rebote del botón en milisegundos
const unsigned long intervalTime = 600;  // Tiempo de espera entre pulsaciones en milisegundos

unsigned long lastDebounceTime = 0;  // Para almacenar el último tiempo en que cambió el estado del botón
unsigned long lastPressTime = 0;  // Para almacenar el último tiempo en que se presionó el botón
unsigned long lastReleaseTime = 0;  // Para almacenar el último tiempo en que se soltó el botón
int buttonState = HIGH;  // Estado actual del botón
int lastButtonState = HIGH;  // Estado anterior del botón
int pressCount = 0;  // Contador de pulsaciones

void setup() {
  pinMode(BTN_PIN, INPUT_PULLUP);
  //pin modes for tx, rx:
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  pinMode(BUSY_PIN,INPUT);

  mySoftwareSerial.begin(9600);
  Serial.begin(115200);

  while (!Serial) {
    ; //Espera por la inicialización de Serial
  }

  D_println(F("Initiating.."));
  //Starting
  checkInitState();
  initLeds();
  // INIT STEP 1 = LEDS Initiated
  while(init_step < 1) delay(1);
  initDFPlayer();
  // INIT STEP 2 = DFPlayer Initiated
  while(init_step < 2) delay(1);
  initNFCReader();
  // INIT STEP 3 = NFCReader Initiated
  while(init_step < 3) delay(1);
  
  D_println(F("----------------------"));
}

void loop() {
  currentLoopTime = millis();
    
  checkButton();
  //checkSoundIsPlaying();

  //if (!isPlaying) {
    readNFC();
  //}
}
// INIT PROCESS
void initLeds() {
  setup_rgb();
  defaultGreenColor();
  D_println(F("Led Strip working"));
  init_step = 1;
}

void initNFCReader() {
  nfc.begin(); //initialization of communication with the module NFC
  
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    D_print("Didn't Find PN53x Module");
    while (1); // Halt
  }
    D_println();
    // Got valid data, print it out!
    D_print("Found chip PN5"); D_println((versiondata >> 24) & 0xFF, HEX);
    D_print("Firmware ver. "); D_print((versiondata >> 16) & 0xFF, DEC);
    D_print('.'); D_println((versiondata >> 8) & 0xFF, DEC);

  // Configure board to read RFID tags
  nfc.SAMConfig();
  nfc.setPassiveActivationRetries(0x00);

  D_println(F("NFC reader working"));
  init_step++;
  checkInitState();
}

void initDFPlayer() {
  D_println();
  D_println(F("DFRobot DFPlayer Mini"));
  D_println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

  D_println(F("DFPlayer Mini online."));
  //myDFPlayer.setTimeout(1000);
  // check errors+
  if ( checkForErrors() != 1 ) {
    D_println(F("[ No errors ]"));

    myDFPlayer.volume(VOLUME_LEVEL);  // Set volume value. From 0 to 29
    delay(500);
    

    // set max timer to wait for the change
    unsigned long start_time = millis();
    while (millis() - start_time < 1000) { // Esperar hasta 200 milisegundos (1000ms = 1 segundo)
      if (myDFPlayer.currentVolume() == VOLUME_LEVEL) {
        break; // Salir del bucle si el volumen se ha configurado correctamente
      }
    }

    D_print("Current Volume : ");
    D_print( myDFPlayer.currentVolume() );
    D_println(F(""));
    D_print("Total Num tracks: ");
    D_print(myDFPlayer.numSdTracks());
    D_println(F(""));

    num_tracks_in_folder = myDFPlayer.numTracksInFolder(actual_folder);

    D_print("Current track : ");
    D_print(myDFPlayer.currentSdTrack());
    D_println(F(""));
    // num_folders = myDFPlayer.numFolders() //contar 1 menos debido a la carpeta de SOUNDS
  } else {
    D_println(F("[ Some errors to fix ]"));
    while (1); // Halt
  }
  init_step++;
  checkInitState();
}
//  --------------

void checkInitState() {
  D_println(F(""));
  D_print("Init Phase: ");
  D_print(init_step);
  D_println(F(""));
}

//DFPlayer FUNCTIONS
int checkForErrors() {
  int has_errors = 0;

  if (!myDFPlayer.begin(mySoftwareSerial)) {  // Use softwareSerial to communicate with mp3.
    D_println(F("Unable to begin:"));
    D_println(F("1.Please recheck the connection!"));
    D_println(F("2.Please insert the SD card!"));
    D_println( myDFPlayer.numSdTracks() ); //read mp3 state
    has_errors = 1;
  }

  if ( myDFPlayer.numSdTracks() == -1) {
    has_errors = 1;
    has_media = false;
    D_println(F("- SD card not found"));
  } else {
    has_media = true;
  }

  return has_errors;
}

void checkSoundIsPlaying() {
  // Lee el estado del pin BUSY_PIN
  int busyPinState = digitalRead(BUSY_PIN);

  if (busyPinState == LOW ){
    isPlaying = true;
  } else if (busyPinState == HIGH ) {
    isPlaying = false;
  }
}

void newSkill_sound() {
  D_println(F(""));
  D_println(F("New Skill Obtained !"));
  myDFPlayer.playFolder(MP3_SOUNDS_FOLDER, 1); //Play the ON SOUND mp3
  actual_track_n = 1;
 // delay(200);
}

void rageShield_sound() {
  myDFPlayer.playFolder(MP3_EFFECTS_FOLDER, 4); //Play the ON SOUND mp3
}

void airStrikeShield_sound() {
  myDFPlayer.playFolder(MP3_EFFECTS_FOLDER, 2); //Play the ON SOUND mp3
}

void prisonShield_sound() {
  myDFPlayer.playFolder(MP3_EFFECTS_FOLDER, 3); //Play the ON SOUND mp3
}

void bashShield_sound() {
  unsigned long currentMillis = millis();

  D_println(F(""));
  D_println(F("Shield Bash!"));
  //1 sec delay for SHIELD BASH EFFECT
  //int counter = 0;
  int bash_delay = 800;
  //counter = currentLoopTime; //equal to actual millis()
  delay(bash_delay);
  myDFPlayer.playFolder(MP3_EFFECTS_FOLDER, 1); //Play the ON SOUND mp3
  actual_track_n = 1;
  delay(50);
}

void alternative_sound(int sound_number) {
  if (isPlaying){
    myDFPlayer.pause(); //Stop SOUND // ERROR - isPlaying ALWAYS TRUE
  } else {
    myDFPlayer.playFolder(MP3_ALTERN_FOLDER, sound_number); //Play SOUND
    delay(10);
  }
}

//NFC FUNCTIONS
void readNFC() {
  boolean success = false;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                       // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  
  if (success) {
    D_print("UID Length: "); D_print(uidLength, DEC); D_println(" bytes");
    D_print("UID Value: ");
    for (uint8_t i = 0; i < uidLength; i++) {
      nuidPICC[i] = uid[i];
      D_print(" "); D_print(uid[i], DEC);
    }
    D_println();
    tagId = tagToString(nuidPICC);
    dispTag = tagId;
    D_print(F("tagId is : "));
    D_println(tagId);
    D_println("");
    
    //if(last_card_UID != dispTag){
      D_println( detectType(dispTag) );
      last_card_UID = dispTag;
    //}

    
    //delay(1000);  // 1 second halt
  } else {
   // PN532 probably timed out waiting for a card
   //D_println("Timed out! Waiting for a card...");
  }
}

String tagToString(byte id[4]) {
 String tagId = "";
 for (byte i = 0; i < 4; i++) {
   if (i < 3) tagId += String(id[i]) + ".";
   else tagId += String(id[i]);
 }
 return tagId;
}

String detectType(String UID) {
  type = "Not recognized";
  if (UID == "4.36.84.50") {
    
    type = "RAGE SHIELD";
    rageShield_sound();
    setColorLedStripMode('R');
    
  } else if (UID == "4.89.171.50") {
    
    type = "AIR STRIKE";
    airStrikeShield_sound();
    setColorLedStripMode('A');
 
  } else if (UID == "4.211.191.50"){
    
    type = "PRISON SHIELD";
    prisonShield_sound();
    setColorLedStripMode('P');
    
  } else if ( UID == "4.125.142.50" || UID == "4.9.73.50") {
    
    type = "learn skill";
    newSkill_sound();
    setColorLedStripMode('N');
    
  } else if (UID == "4.239.73.50") {
    //METAL GEAR RISING MEME
    alternative_sound(1);
    type = "altern";
  } else if (UID == "4.71.78.50") {
    //OPENING THEME
    alternative_sound(2);
    type = "altern";
  } else if (UID == "4.61.172.50" ) {
    alternative_sound(3);
    type = "altern";
  } else if (UID == "4.88.188.50" ) {
    alternative_sound(5);
    type = "altern";
  } else if (UID =="4.199.171.50") {
    setColorLedStripMode('N');
  }
  
  return type;
}

boolean array_includes(int array[], int element, int array_size) {
  for (int i = 0; i < array_size; i++) {
    if (array[i] == element) {
      return true;
    }
  }
  return false;
}

void pause_delay(int delay_time = 2000) {
  auto lastLoopTime = millis();
  while(millis() - lastLoopTime < delay_time) {
    delay(3);
  }
}

//NEOPIXEL FUNCTIONS
void setup_rgb() {
  strip.begin();

  while(strip.numPixels() == 0) {
    ; // wait
  }

  strip.setBrightness(BRIGHTNESS);
}

byte fadeCycleTime(int fade_time = 1000, byte fade_speed = 1) {
  //tiempo de fade, fadespeed , rgbmax
  byte cycle_delay = fade_time * fade_speed / 255;

  return cycle_delay;
}

void fadeToColor(byte r, byte g, byte b, byte cycle_delay = 3) {
    //tiempo de fade, fadespeed , rgbmax
  
    //LEDS_COLOR - actual color
    NextCOLOR = {r, g, b};
  
    while(LEDS_COLOR.r != NextCOLOR.r || LEDS_COLOR.g != NextCOLOR.g || LEDS_COLOR.b != NextCOLOR.b ){
      if( LEDS_COLOR.r > NextCOLOR.r ){ LEDS_COLOR.r--; } else if( LEDS_COLOR.r < NextCOLOR.r ){ LEDS_COLOR.r++; } 
      if( LEDS_COLOR.g > NextCOLOR.g ){ LEDS_COLOR.g--; } else if( LEDS_COLOR.g < NextCOLOR.g ){ LEDS_COLOR.g++; }
      if( LEDS_COLOR.b > NextCOLOR.b ){ LEDS_COLOR.b--; } else if( LEDS_COLOR.b < NextCOLOR.b ){ LEDS_COLOR.b++; }

      for (int i = 0; i < 10; i++ ) {
        setPixel(i, LEDS_COLOR.r, LEDS_COLOR.g, LEDS_COLOR.b);
      }
      strip.show();
      delay(cycle_delay );
    }
  
}

void defaultGreenColor() {
  // Fade IN - GREEN
  setAll(0, 220, 0);
}

void rageShield_colors() {
  int r1 = 0;
  int g1 = 0;
  int b1 = 0;
  unsigned long lastLoopTime = 0;  // Usar unsigned long para millis()
  int interval = 0;
  unsigned long cycle_delay = 0;

  // A - inicio de la corrupción (4 segundos)
  lastLoopTime = millis();
  interval = 1000;
  byte random_led = random(0, 11);

  while (millis() - lastLoopTime < interval) {
    strip.setPixelColor(random_led, 226, 21, 35);
    strip.show();
    delay(3);
  }
  D_println(F("First animation done"));

  lastLoopTime = millis();
  interval = 2000;

  while (millis() - lastLoopTime < interval) {
    const int arraySize = 2;
    int chosen_leds[arraySize];
    int value;

    for (int i = 0; i < arraySize; i++) {
      do {
        value = random(0, 11);
      } while (array_includes(chosen_leds, value, arraySize));
      chosen_leds[i] = value;
    }

    int r = 226, g = 21, b = 35;
    for (int i = 0; i < arraySize; i++) {
      int flicker = random(0, 55);
      r1 = r - flicker;
      g1 = g - flicker;
      b1 = b - flicker;
      if (g1 < 0) g1 = 0;
      if (r1 < 0) r1 = 0;
      if (b1 < 0) b1 = 0;
      strip.setPixelColor(chosen_leds[i], r1, g1, b1);
    }
    strip.show();
    delay(random(10, 100));
  }
  D_println(F("Second animation done"));

  // B - Fade de verde a rosado
  cycle_delay = fadeCycleTime(1000);
  fadeToColor(226, 21, 35, cycle_delay);

  // C - Flickering de llamas (7 segundos)
  lastLoopTime = millis();
  interval = 7000;

  while (millis() - lastLoopTime < interval) {
    int r = 226, g = 21, b = 35;
    for (int i = 0; i < 10; i++) {
      int flicker = random(0, 55);
      r1 = r - flicker;
      g1 = g - flicker;
      b1 = b - flicker;
      if (g1 < 0) g1 = 0;
      if (r1 < 0) r1 = 0;
      if (b1 < 0) b1 = 0;
      strip.setPixelColor(i, r1, g1, b1);
    }
    strip.show();
    delay(random(10, 100));
  }
  D_println(F("Third animation done"));

  lastLoopTime = millis();
  interval = 1400;

  while (millis() - lastLoopTime < interval) {
    int r = 226, g = 21, b = 35;
    for (int i = 0; i < 10; i++) {
      int flicker = random(0, 55);
      r1 = r - flicker;
      g1 = g - flicker;
      b1 = b - flicker;
      if (g1 < 0) g1 = 0;
      if (r1 < 0) r1 = 0;
      if (b1 < 0) b1 = 0;
      strip.setPixelColor(i, r1, g1, b1);
    }
    strip.show();
    delay(random(10, 100));
  }
  D_println(F("Fourth animation done"));

  LEDS_COLOR.r = r1;
  LEDS_COLOR.g = g1;
  LEDS_COLOR.b = b1;

  // D - Fade a ROJO
  cycle_delay = fadeCycleTime(2000);
  fadeToColor(255, 0, 0, cycle_delay);
}

void setColorLedStripMode(char color) {
  auto cycle_delay = 1000;
  
  switch(color) {
    case 'R':
      rageShield_colors();
    break;
    case 'N':
    //NEW SKILL
      cycle_delay = fadeCycleTime(1200);
      fadeToColor(59, 235, 71, cycle_delay);
      pause_delay(1000);
      cycle_delay = fadeCycleTime(1400);
      fadeToColor(0, 220, 0, cycle_delay);
    break;
    case 'A':
    //AIR STRIKE
      cycle_delay = fadeCycleTime(1200);
      fadeToColor(0, 255, 25, cycle_delay);
      pause_delay(3000);
      cycle_delay = fadeCycleTime(1400);
      fadeToColor(0, 220, 0, cycle_delay);
    break;
    case 'B':
      cycle_delay = fadeCycleTime(500);
      fadeToColor(0, 0, 255, cycle_delay);
    break;
    case 'P': //Shield Prison
      cycle_delay = fadeCycleTime(500);
      fadeToColor(255, 233, 0, cycle_delay);
      pause_delay(3000);
      cycle_delay = fadeCycleTime(800);
      fadeToColor(0, 220, 0 , cycle_delay);
    break;
    default:
      cycle_delay = fadeCycleTime(500);
      fadeToColor(0, 220, 0, cycle_delay);
     break;
  }
}

void setAll(byte red, byte green, byte blue) {
  for (int i = 0; i < LED_COUNT; i++ ) {
    setPixel(i, red, green, blue);
  }
  strip.show();
}

void setPixel(int Pixel, byte red, byte green, byte blue) {
  // NeoPixel
  strip.setPixelColor(Pixel, strip.Color(red, green, blue));
}

//Button Functions
void checkButton() {
  int reading = digitalRead(BTN_PIN);

  // Verificar si ha pasado el tiempo de rebote
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  // Si ha pasado el tiempo de rebote, actualizar el estado del botón
  if ((millis() - lastDebounceTime) > debounceTime) {
    // Si el estado del botón ha cambiado
    if (reading != buttonState) {
      buttonState = reading;

      // Si el botón está presionado
      if (buttonState == LOW) {
        pressCount++;
        lastPressTime = millis();
      }
    }
  }

  // Si ha pasado el intervalo de tiempo sin pulsaciones adicionales
  if ((millis() - lastPressTime) > intervalTime && pressCount > 0) {
    D_print("Número de pulsaciones detectadas: ");
    D_println(pressCount);

    switch (pressCount) {
      case 1:
        bashShield_sound();
        break;
      case 2:
        airStrikeShield_sound();
        setColorLedStripMode('A');
        break;
      case 3:
        prisonShield_sound();
        setColorLedStripMode('P');
        break;
      case 4:
        rageShield_sound();
        setColorLedStripMode('R');
        break;
      default:
        
        break;
    }
    pressCount = 0;  // Resetear el contador de pulsaciones
  }

  lastButtonState = reading;  // Guardar el estado actual del botón para la próxima iteración

}
