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
#define VOLUME_LEVEL 10
#define MP3_SOUNDS_FOLDER 10
SoftwareSerial mySoftwareSerial(rxPin, txPin); // RX, TX
DFPlayerMini_Fast myDFPlayer;
boolean has_media = true;
int num_tracks_in_folder = 0;
int actual_folder = 1;
int actual_track_n = 0;
boolean initSound = false;

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
NfcAdapter nfc = NfcAdapter(pn532_i2c);

// NEOPIXEL
#include <Adafruit_NeoPixel.h>
// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN   12
// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 10
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
#define BRIGHTNESS 170
char stripColor = 'G';

//global RGB values
int red = 0;
int green = 0;
int blue = 0;
int rgb_max = 0;
//Por tal de no quedarse atascado en una animación de leds, se comprueban estados actuales y procesos pendientes
boolean anim_started = 0;
boolean anim_finished = 0;
int anim_code = 0;

//Initiation steps
int init_step = 0;
boolean initiated = false;

void setup() {
  // define pin modes for tx, rx:
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);

  mySoftwareSerial.begin(9600);
  Serial.begin(115200);
  button.setDebounceTime(50);

  delay(50);
  Serial.println(F("Initiating.."));
  //Starting
  initLeds();
  while(init_step < 1) delay(10);
  initDFPlayer();
  while(init_step < 2) delay(10);
  initNFCReader();

}

void loop() {
  button.loop();
  int btnState = button.getState();
  if(button.isPressed() )
    Serial.println(F("The button is pressed"));
    
  if(init_step == 3){
    readNFC();
  }

  if(anim_started == 1 && anim_finished == 0){
    //continue started animation..
    continue_anim();
  }

}



void initLeds(){
  setup_rgb();
  defaultGreenColor();
  Serial.println(F("Led Strip working"));
  init_step = 1;
}

void initNFCReader(){
  nfc.begin();//initialization of communication with the module NFC

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

    Serial.println();
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


void playNewSkillSound() {
  Serial.println(F(""));
  Serial.println(F("New Skill Obtained !"));
  myDFPlayer.playFolder(MP3_SOUNDS_FOLDER, 1); //Play the ON SOUND mp3
  actual_track_n = 1;
  initSound = true;
  delay(200);
}


//NFC FUNCTIONS
void readNFC()
{
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                       // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  if (nfc.tagPresent())
  {
    NfcTag tag = nfc.read(); //reading the NFC card or tag
    Serial.println(F(""));

    if ( last_card_UID == tag.getUidString() ) {
     new_card = false;
    } else {
      new_card = true;
    }

    if (new_card) {
      last_card_UID = tag.getUidString();

//      Serial.println("");
//      Serial.print("Id: ");
//      Serial.print(  tag.getUidString() );
      Serial.println( detectType(tag.getUidString()) );
//      Serial.println();

      delay(100);  // 1 second halt
    }

  } else {
    waiting = true;
    // PN532 probably timed out waiting for a card
  }

  if (waiting) {
      //Serial.print(".");
  }else{
    //Serial.println("");
  }

}

void animate(int animation = 0,int r = 0, int g = 0, int b = 0){
  //asignar RGB
  
  
  

  

  switch(animation){
    case 1:
    //Fade GREEN - initial
    fadeBasicGreen();
    break;
  }
  
}

String detectType(String UID) {
  type = "Not recognized";
  if (UID == "B4 67 C8 73") {
    
    type = "red";
    playNewSkillSound();
    //rageShield();
    animate(1);
    
  } else if (UID == "C3 F0 48 92") {
    
    type = "blue";
    playNewSkillSound();
    prisonShield();
 
  } else {
    //(UID == "63 1C 54 A7")
    
    type = "green";
    playNewSkillSound();
    setColorLedStrip('R');
    
  }
  return type;
}


//NEOPIXEL FUNCTIONS
void setup_rgb() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
}



void continue_anim(){
  animate(anim_code);
}

void fadeBasicGreen(){
  
  if(anim_started == 0){
    anim_started = 1;
    anim_finished = 0;
    red = 0;
    green = 255;
    blue = 0;
    int rgb[3] = {red, green, blue};
    rgb_max = getHighest(rgb);
  }
  
  for (int k = 0; k < rgb_max; k++) {
    setAll(0, k, 0);
    strip.show();
    delay(3);
  }
}

void defaultGreenColor() {
  // Fade IN - GREEN
  for (int k = 0; k < 256; k++) {
    setAll(0, k, 0);
    strip.show();
    delay(3);
  }
}

void newSkill(){
  //Green basic effect
  for (int k = 0; k < 256; k++) {
    setAll(0, k, 0);
    strip.show();
    delay(3);
  }
}

void rageShield(){

  for(int secs = 0; secs < 300 ; secs++) {
    //Fire Effect
    //  Regular (orange) flame:
    int r = 226, g = 21, b = 35;
    //  Purple flame:
    //  int r = 158, g = 8, b = 148;
    
    //  Flicker, based on our initial RGB values
    for(int i=0; i< 10; i++) {
      int flicker = random(0,55);
      int r1 = r-flicker;
      int g1 = g-flicker;
      int b1 = b-flicker;
      if(g1<0) g1=0;
      if(r1<0) r1=0;
      if(b1<0) b1=0;
      strip.setPixelColor(i,r1,g1, b1);
    }
    strip.show();
    //  Adjust the delay here, if you'd like.  Right now, it randomizes the 
    //  color switch delay to give a sense of realism
    delay(random(10,100));
  }
  //set all red, stable
  setColorLedStrip('R');
  
}

void prisonShield(){
  int red = 0;
  int green = 0;
  int blue = 0;
  int max_red = 0;
  int max_green = 255;
  int max_blue = 245;
  int highest = red + green + blue / 3;
  
  for (int k = 0; k < highest; k++) {
    
    if(green < max_green) green++;
    if(blue < max_blue) blue++;
    setAll(red, green, 0);
    strip.show();
    delay(3);
  }
}



void setColorLedStrip(char color){
  switch(color){
    case 'R':
      for (int k = 0; k < 256; k++) {
        setAll(k, 0, 0);
        strip.show();
        delay(3);
      }
    break;
    case 'G':
      for (int k = 0; k < 256; k++) {
        setAll(0, k, 0);
        strip.show();
        delay(3);
      }
    break;
    case 'B':
      for (int k = 0; k < 256; k++) {
        setAll(0, 0, k);
        strip.show();
        delay(3);
      }
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



int getHighest(int arr[3]){
   int maxVal = arr[0];
   int minVal = arr[0];
   Serial.print("Size of myArray is: ");
   Serial.println(sizeof(arr));

   for (int i = 0; i < (sizeof(arr) / sizeof(arr[0])); i++) {
      if (arr[i] > maxVal) {
         maxVal = arr[i];
      }
      if (arr[i] < minVal) {
         minVal = arr[i];
      }
   }
   Serial.print("Max is: ");
   Serial.print(maxVal);
   return maxVal;
}
