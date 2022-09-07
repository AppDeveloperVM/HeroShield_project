//BASIC
#include "Arduino.h"
#include "SoftwareSerial.h"

//Buttons
#include <ezButton.h>
ezButton button(9);

//DFPLAYER
#include <DFPlayerMini_Fast.h>
#define rxPin 10
#define txPin 11
#define BUSY_PIN 7
#define VOLUME_LEVEL 6
#define MP3_SOUNDS_FOLDER 10 //Init sound
#define MP3_EFFECTS_FOLDER 01 //Shield Bash Sound
#define MP3_ALTERN_FOLDER 02 //Alternative Sounds
SoftwareSerial mySoftwareSerial(rxPin, txPin); // RX, TX
DFPlayerMini_Fast myDFPlayer;
boolean has_media = true;
int num_tracks_in_folder = 0;
int actual_folder = 1;
int actual_track_n = 0;
boolean initSound = false;
boolean isPlaying = false;

// NFC libs
//including the library for I2C communication
#include <Wire.h>
//including the library for I2C communication with the PN532 module, library for the PN532 module and the library for the NFC
#include <PN532_I2C.h>
#include <PN532.h>
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
// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN   12
// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 10
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
#define BRIGHTNESS 100
int red = 0;
int green = 0;
int blue = 0;
char stripColor = 'G';

// declare structure for rgb variable
struct rgbColor {
  byte r;    // red value 0 to 4095
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
unsigned long previousBtnMillis = 0;

void setup() {
  pinMode(9, INPUT_PULLUP);
  //pin modes for tx, rx:
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  pinMode(BUSY_PIN,INPUT);

  mySoftwareSerial.begin(9600);
  Serial.begin(115200);

  delay(50);
  Serial.println(F("Initiating.."));
  //Starting
  initLeds();
  while(init_step < 1) delay(10);
  initDFPlayer();
  while(init_step < 2) delay(10);
  initNFCReader();
  Serial.println(F("----------------------"));
}

void loop() {
  currentLoopTime = millis();
    
  checkButton();
  checkSoundIsPlaying();
}
// INIT PROCESS
void initLeds(){
  setup_rgb();
  defaultGreenColor();
  Serial.println(F("Led Strip working"));
  init_step = 1;
}

void initNFCReader(){
  nfc.begin();//initialization of communication with the module NFC
  
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't Find PN53x Module");
    //while (1); // Halt
  }
    Serial.println();
  // Got valid data, print it out!
    Serial.print("Found chip PN5"); Serial.println((versiondata >> 24) & 0xFF, HEX);
    Serial.print("Firmware ver. "); Serial.print((versiondata >> 16) & 0xFF, DEC);
    Serial.print('.'); Serial.println((versiondata >> 8) & 0xFF, DEC);

  // Configure board to read RFID tags
  nfc.SAMConfig();
  nfc.setPassiveActivationRetries(0x00);

  Serial.println(F("NFC reader working"));
  init_step++;
}

void initDFPlayer(){
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));


  Serial.println(F("DFPlayer Mini online."));
  // check errors+
  if ( checkForErrors() != 1 ) {
    Serial.println(F("[ No errors ]"));

    myDFPlayer.volume(VOLUME_LEVEL);  //Set volume value. From 0 to 30
    delay(200);

    Serial.print("Current Volume : ");
    Serial.print( myDFPlayer.currentVolume() );
    Serial.println(F(""));
    Serial.print("Total Num tracks: ");
    Serial.print(myDFPlayer.numSdTracks());
    Serial.println(F(""));

    num_tracks_in_folder = myDFPlayer.numTracksInFolder(actual_folder);

    Serial.print("Current track : ");
    Serial.print(myDFPlayer.currentSdTrack());
    Serial.println(F(""));
    // num_folders = myDFPlayer.numFolders() //contar 1 menos debido a la carpeta de SOUNDS
  } else {
    Serial.println(F("[ Some errors to fix ]"));
  }
  init_step++;
}

//DFPlayer FUNCTIONS
int checkForErrors() {
  int has_errors = 0;

  if ( !myDFPlayer.begin(mySoftwareSerial) ) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    Serial.println( myDFPlayer.numSdTracks() ); //read mp3 state

    has_errors = 1;

    while (true);
  }

  if ( myDFPlayer.numSdTracks() == -1) {
    has_errors = 1;
    has_media = false;
    Serial.println(F("- SD card not found"));
  }

  return has_errors;
}

void checkSoundIsPlaying(){
  if(digitalRead(BUSY_PIN) == LOW ){
    isPlaying = true;
  }else if( digitalRead(BUSY_PIN) == HIGH ) {
    isPlaying = false;
  }
}



void newSkill_sound() {
  Serial.println(F(""));
  Serial.println(F("New Skill Obtained !"));
  myDFPlayer.playFolder(MP3_SOUNDS_FOLDER, 1); //Play the ON SOUND mp3
  actual_track_n = 1;
  initSound = true;
  delay(200);
}

void rageShield_sound(){
  myDFPlayer.playFolder(MP3_EFFECTS_FOLDER, 4); //Play the ON SOUND mp3
}

void airStrikeShield_sound(){
  myDFPlayer.playFolder(MP3_EFFECTS_FOLDER, 2); //Play the ON SOUND mp3
}

void prisonShield_sound(){
  myDFPlayer.playFolder(MP3_EFFECTS_FOLDER, 3); //Play the ON SOUND mp3
}

void bashShield_sound(){
  Serial.println(F(""));
  Serial.println(F("Shield Bash!"));
  //1 sec delay for SHIELD BASH EFFECT
  //int counter = 0;
  int bash_delay = 800;
  //counter = currentLoopTime;//equal to actual millis()
  delay(bash_delay);
  myDFPlayer.playFolder(MP3_EFFECTS_FOLDER, 1); //Play the ON SOUND mp3
  actual_track_n = 1;
  initSound = true;
  delay(200);
}

void alternative_sound(int sound_number){
  if(isPlaying){
    myDFPlayer.pause(); //Stop SOUND
  }else{
    myDFPlayer.playFolder(MP3_ALTERN_FOLDER, sound_number); //Play SOUND
    delay(10);
  }
}

//NFC FUNCTIONS
void readNFC()
{
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                       // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  
  if (success) {
    Serial.print("UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i = 0; i < uidLength; i++) {
      nuidPICC[i] = uid[i];
      Serial.print(" "); Serial.print(uid[i], DEC);
    }
    Serial.println();
    tagId = tagToString(nuidPICC);
    dispTag = tagId;
    Serial.print(F("tagId is : "));
    Serial.println(tagId);
    Serial.println("");
    
    //if(last_card_UID != dispTag){
      Serial.println( detectType(dispTag) );
      last_card_UID = dispTag;
    //}

    
    delay(1000);  // 1 second halt
    
  } else {
   // PN532 probably timed out waiting for a card
   //Serial.println("Timed out! Waiting for a card...");

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
    
    type = "red";
    rageShield_sound();
    setColorLedStrip('R');
    
  } else if (UID == "4.89.171.50") {
    
    type = "green";
    //newSkill_sound();
    airStrikeShield_sound();
    //newSkill();
    setColorLedStrip('G');
 
  } else if( UID == "4.211.191.50" ){
    
    type = "yellow";
    //playNewSkillSound();
    prisonShield_sound();
    setColorLedStrip('Y');

  } else if(  UID == "4.239.73.50") {
    alternative_sound(1);
    type = "altern";
  }
  return type;
}


//NEOPIXEL FUNCTIONS
void setup_rgb() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
}

byte fadeCycleTime(int fade_time = 1000, byte fade_speed = 1){
  //tiempo de fade, fadespeed , rgbmax
  byte cycle_delay = fade_time * fade_speed / 255;

  return cycle_delay;
}

void fadeToColor(byte r, byte g, byte b, byte cycle_delay = 3){
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
  //setColorLedStrip('G');
  setAll(0, 220, 0);
}

boolean array_includes(int array[], int element, int array_size){
  for(int i = 0; i < array_size; i++){
    if( array[i] == element ){
      return true;
    }
  }
  return false;
}

void rageShield_(){
  int r1 = 0;
  int g1 = 0;
  int b1 = 0;
  auto lastLoopTime = 0;
  int interval = 0;
  byte cycle_delay = 0;

  //Etapas 
  //A - inicio de la corrupción, solamente unos pocos efectos, desde el color verde al rosado / rojo ( 4 segs ) 
  //B - Fade de verde a rosado / rojo ( 1 - 2 segs )
  //C - Flickering de llamas ( 5 / 6 segs )
  //D - Fade a ROJO

  //A - inicio de la corrupción ( 4 segundos )
  //parte 1
  lastLoopTime = millis();
  interval = 1000;
  byte random_led = random(0,11);
  
  while(millis() - lastLoopTime < interval){
    //1 led ( 1 seg)
    //random led    
    strip.setPixelColor(random_led, 226, 21, 35);
    strip.show();
    delay(3);
  }
  Serial.println(F("First animation done"));

  lastLoopTime = millis();
  interval = 2000;
  
  while(millis() - lastLoopTime < interval){
    
    //2 leds ( 2 segs )
    const int arraySize = 2;
    int exclusions[arraySize - 1];  
    int chosen_leds[arraySize ];
    int index = 0;
    int value;

    for(int i=0; i< arraySize; i++){
      do {
        value = random(0,11);
      } while( array_includes(chosen_leds, value, arraySize ) );

      chosen_leds[i] = value;
    }
 
    //Fire Effect
    //  Regular (orange) flame:
    int r = 226, g = 21, b = 35;
    //  Purple flame:
    //  int r = 158, g = 8, b = 148;
    
    //  Flicker, based on our initial RGB values
    for(int i=0; i< arraySize; i++) {
      int flicker = random(0,55);
      r1 = r-flicker;
      g1 = g-flicker;
      b1 = b-flicker;
      if(g1<0) g1=0;
      if(r1<0) r1=0;
      if(b1<0) b1=0;
      strip.setPixelColor(chosen_leds[i] , r1 , g1, b1);
    }
    strip.show();
    //  Adjust the delay here, if you'd like.  Right now, it randomizes the 
    //  color switch delay to give a sense of realism
    delay(random(10,100));
  }
  Serial.println(F("Second animation done"));

  //B - Fade de verde a rosado
  cycle_delay = fadeCycleTime(1000); 
  fadeToColor(226, 21, 35, cycle_delay); // rosado

  //parte 2
  //C - Flickering de llamas
  lastLoopTime = millis();
  interval = 4000;
  
  while(millis() - lastLoopTime < interval){
    //Fire Effect
    //  Regular (orange) flame:
    int r = 226, g = 21, b = 35;
    //  Purple flame:
    //  int r = 158, g = 8, b = 148;
    
    //  Flicker, based on our initial RGB values
    for(int i=0; i< 10; i++) {
      int flicker = random(0,55);
      r1 = r-flicker;
      g1 = g-flicker;
      b1 = b-flicker;
      if(g1<0) g1=0;
      if(r1<0) r1=0;
      if(b1<0) b1=0;
      strip.setPixelColor(i, r1 , g1, b1);
    }
    strip.show();
    //  Adjust the delay here, if you'd like.  Right now, it randomizes the 
    //  color switch delay to give a sense of realism
    delay(random(10,100));
  }

  Serial.println(F("Third animation done"));

  lastLoopTime = millis();
  interval = 1400;
  
  if(millis() - lastLoopTime < interval){
  //for(int secs = 0; secs < 300 ; secs++) {
   
    //Fire Effect
    //  Regular (orange) flame:
    int r = 226, g = 21, b = 35;
    //  Purple flame:
    //  int r = 158, g = 8, b = 148;
    
    //  Flicker, based on our initial RGB values
    for(int i=0; i< 10; i++) {
      int flicker = random(0,55);
      r1 = r-flicker;
      g1 = g-flicker;
      b1 = b-flicker;
      if(g1<0) g1=0;
      if(r1<0) r1=0;
      if(b1<0) b1=0;
      strip.setPixelColor(i, r1 , g1, b1);
    }
    strip.show();
    //  Adjust the delay here, if you'd like.  Right now, it randomizes the 
    //  color switch delay to give a sense of realism
    delay(random(10,100));
  }

  Serial.println(F("Fourth animation done"));

  LEDS_COLOR.r = r1;
  LEDS_COLOR.g = g1;
  LEDS_COLOR.b = b1;

  //D - Fade a ROJO
  cycle_delay = fadeCycleTime(2000);
  fadeToColor(255, 0, 0, cycle_delay);
}





void pause_delay(int delay_time = 2000){
  auto lastLoopTime = millis();
  while(millis() - lastLoopTime < delay_time){
    delay(3);
  }
}

void setColorLedStrip(char color){
  byte cycle_delay = 1000;
  
  switch(color){
    case 'R':
      rageShield_();
    break;
    case 'G':
      cycle_delay = fadeCycleTime(1200);
      fadeToColor(50, 254, 30, cycle_delay);
      pause_delay(3000);
      cycle_delay = fadeCycleTime(1400);
      fadeToColor(0, 220, 0, cycle_delay);
    break;
    case 'B':
      cycle_delay = fadeCycleTime(500);
      fadeToColor(0, 0, 255, cycle_delay);
    break;
    case 'Y': //Shield Prison
      cycle_delay = fadeCycleTime(500);
      fadeToColor(255, 233, 0, cycle_delay);
      pause_delay(3000);
      cycle_delay = fadeCycleTime(800);
      fadeToColor(0, 220, 0 , cycle_delay);
      
    break;
  }
}


void setAll(byte red, byte green, byte blue) {
  for (int i = 0; i < 10; i++ ) {
    setPixel(i, red, green, blue);
  }
  strip.show();
}

void setPixel(int Pixel, byte red, byte green, byte blue) {

  // NeoPixel
  strip.setPixelColor(Pixel, strip.Color(red, green, blue));
}

//Button Functions
void checkButton(){
  int mode = digitalRead(9);
  
  delay(10); // quick and dirty debounce filter
  if(lastStatus != mode){
    lastStatus = mode;

    if(mode == LOW){
      //modes defined by time the button is pressed
      //Serial.println(F("Basic button press"));
      
    } else { 
      //Serial.println(F("Button released"));
      auto interval = currentLoopTime - previousBtnMillis;

      if( currentLoopTime  >= 2000) { // check if 1000ms passed)

        // Button released, check which function to launch
        if (interval < 100)
        {} // ignore a bounce
        else if (interval < POWER_ACTION_TIME ){ //CHANGE MODE
            Serial.println(F("CHANGE MODE triggered"));
            bashShield_sound();
        } else if (interval >= POWER_ACTION_TIME) //POWER - ON / OFF
            Serial.println(F("POWER triggered"));
        else {
            //
        }
        Serial.println(interval);
      }
    }
    previousBtnMillis = currentLoopTime;
  } else {
    readNFC(); 
  }

}
