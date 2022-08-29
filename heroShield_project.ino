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
#define VOLUME_LEVEL 17
#define MP3_SOUNDS_FOLDER 10 //Init sound
#define MP3_EFFECTS_FOLDER 01 //Shield Bash Sound
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
#define BRIGHTNESS 200
int red = 0;
int green = 0;
int blue = 0;
char stripColor = 'G';

//Initiation steps
int init_step = 0;
boolean initiated = false;

//MODE - defined by Button action
int lastStatus = 0;
int mode = 0;

#define SEC 1000
#define POWER_ACTION_TIME 1100
#define SWITCH_ACTION_TIME 1000
auto currentLoopTime = 0;
unsigned long previousBtnMillis = 0;
//error with first check


void setup() {
  pinMode(9, INPUT_PULLUP);
  
  
  // define pin modes for tx, rx:
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);

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

void playNewSkillSound() {
  Serial.println(F(""));
  Serial.println(F("New Skill Obtained !"));
  myDFPlayer.playFolder(MP3_SOUNDS_FOLDER, 1); //Play the ON SOUND mp3
  actual_track_n = 1;
  initSound = true;
  delay(200);
}

void playSoundBash(){
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
    
    if(last_card_UID != dispTag){
      Serial.println( detectType(dispTag) );
      last_card_UID = dispTag;
    }

    
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
    //playNewSkillSound();
    //rageShield();
    setColorLedStrip('R');
    myDFPlayer.playFolder(MP3_EFFECTS_FOLDER, 4);
    
    
  } else if (UID == "4.89.171.50") {
    
    type = "green";
    playNewSkillSound();
    //newSkill();
    setColorLedStrip('G');
    
 
  } else if( UID == "4.211.191.50" ){
    
    type = "blue";
    playNewSkillSound();
    //prisonShield();
    setColorLedStrip('B');

  }
  return type;
}


//NEOPIXEL FUNCTIONS
void setup_rgb() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
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
  strip.setBrightness(250);
  strip.show();
}

void rageShield(int r = 0, int g = 0, int b = 0){
  if(red != 0) r = red;
  if(green != 0) g = green;
  if(blue != 0) b = blue;
  //254,0,0
    while(r < 254 ){
      if(r < 254) r++;
      setAll(r, g, b);
      strip.show();
      delay(3);
    }
}

void rageShield_(){
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
}

void prisonShield(){
  red = 0;
  green = 0;
  blue = 0;
  //251,245,10
    while(red < 251 || green < 245 || blue < 10){
      if(red < 251) red++;
      if(green < 254) green++;
      if(blue < 10) blue++;
      setAll(red, green, blue);
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
            playSoundBash();
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
