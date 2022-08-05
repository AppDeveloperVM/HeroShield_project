//BASIC
#include "Arduino.h"
#include "SoftwareSerial.h"

//DFPLAYER
#include <DFPlayerMini_Fast.h>
//eZButton
#include <ezButton.h>

// NFC libs
//including the library for I2C communication
#include <Wire.h>
//including the library for I2C communication with the PN532 module, library for the PN532 module and the library for the NFC
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>

// NEOPIXEL
#include <Adafruit_NeoPixel.h>
// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN   12
// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 10
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed
#define BRIGHTNESS 150

#define rxPin 10
#define txPin 11

#define POWER_BUTTON 4
//ezButton POWER_BUTTON(6);
ezButton NEXT_BUTTON(5);
ezButton CHANGE_FOLDER(6);
//ezButton PAUSE_BUTTON(9);

#define VLM_PIN A0
#define SAMPLES 10 //average of x values to stabilize reading
#define BUSY_PIN 2
#define VOLUME_LEVEL 23 // 0 - 30 ( 18 is a good level )
#define MAX_VLM_LVL 17
#define MP3_SOUNDS_FOLDER 10

//MP3 config
int fadeDuration = 800; //LEDS fade duration in ms

int num_tracks_in_folder = 0;
int num_folders = 2;
int actual_track_n = 0;
int actual_folder = 1;
int next_folder = 1;

//NFC config
String tagId = "None", dispTag = "None";
byte nuidPICC[4];
String last_UID;
boolean waiting = false;
int waiting_count = 0;
String type = "Not recognized";

String last_card_UID = "";
boolean new_card = false;

boolean nfc_found = false;
boolean initiated = false;

//constructors for the library for I2C communication with the module and for the NFC
PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);


SoftwareSerial mySoftwareSerial(rxPin, txPin); // RX, TX
DFPlayerMini_Fast myDFPlayer;

//microphone state
boolean lastPowerState = HIGH; // being INPUT_PULLUP, IT IS REVERSED
int powerState = HIGH;
int OnState = LOW;
int OffState = HIGH;

int powerStatus = false;
int On = true;
int Off = false;

boolean isOn = false;
boolean isPlaying = false;
boolean initSound = false;
boolean folder_changed = false;
boolean has_media = true;

int current_volume = 0;
int inputVolume = 0;
int outputVolume = 0;

void setup()
{
  // define pin modes for tx, rx:
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);

  //POWER_BUTTON.setDebounceTime(50);
  CHANGE_FOLDER.setDebounceTime(20);
  NEXT_BUTTON.setDebounceTime(50);
  //PAUSE_BUTTON.setDebounceTime(50);

  current_volume = VOLUME_LEVEL;

  mySoftwareSerial.begin(9600);
  Serial.begin(115200);

  //Starting
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));


  Serial.println(F("DFPlayer Mini online."));
  // check errors+
  if ( checkForErrors() != 1 ) {
    Serial.println("[ No errors ]");

    Serial.println();
    myDFPlayer.volume(VOLUME_LEVEL);  //Set volume value. From 0 to 30
    delay(200);

    Serial.print("Current Volume : ");
    Serial.print( myDFPlayer.currentVolume() );
    Serial.println();
    Serial.print("Total Num tracks: ");
    Serial.print(myDFPlayer.numSdTracks());
    Serial.println();

    num_tracks_in_folder = myDFPlayer.numTracksInFolder(actual_folder);

    Serial.print("Current track : ");
    Serial.print(myDFPlayer.currentSdTrack());
    Serial.println();
    // num_folders = myDFPlayer.numFolders() //contar 1 menos debido a la carpeta de SOUNDS
  } else {
    Serial.println("[ Some errors to fix ]");
  }

  
  //Wire.begin();
  
  nfc.begin();//initialization of communication with the module NFC
  while (!Serial) delay(10); 
  Serial.println("NFC reader working");

  
  
}

void loop()
{

  if(initiated == false){
    initiated = true;
    setup_rgb();
    defaultGreenColor();
  }
  
  readNFC();

}



void Initiation() {
  Serial.println();
  Serial.println(F("STARTING.."));
  isOn = true;

  myDFPlayer.playFolder(MP3_SOUNDS_FOLDER, 1); //Play the ON SOUND mp3
  actual_track_n = 1;
  initSound = true;
  delay(200);
  //fadeLed(digitalRead(LED_A));
}

void setup_rgb() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show();
}

void defaultGreenColor() {
  // Fade IN - GREEN
  for (int k = 0; k < 256; k++) {
    setAll(0, k, 0);
    strip.show();
    delay(3);
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

void playNewSkillSound() {
  Serial.println();
  Serial.println(F("New Skill Obtained !"));
  myDFPlayer.playFolder(MP3_SOUNDS_FOLDER, 1); //Play the ON SOUND mp3
  actual_track_n = 1;
  initSound = true;
  delay(200);
}

String detectType(String UID) {
  type = "Not recognized";
  if (UID == "B4 67 C8 73") {
    type = "red";

    for (int k = 0; k < 256; k++) {
      setAll(k, 0, 0);
      strip.show();
      delay(3);
    }

    playNewSkillSound();
  } else if (UID == "C3 F0 48 92") {
    type = "blue";

    for (int k = 0; k < 256; k++) {
      setAll(0, 0, k);
      strip.show();
      delay(3);
    }

    playNewSkillSound();
  } else if (UID == "63 1C 54 A7") {
    type = "reset";

    for (int k = 0; k < 256; k++) {
      setAll(0, 0, k);
      strip.show();
      delay(3);
    }

    playNewSkillSound();
  }
  return type;
}



// LED functions ---------------------------------




// NFC specific Functions ------------------------

void readNFC()
{
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                       // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  if (nfc.tagPresent())
  {
    //    Serial.println("");
    //    waiting = false;
    //    waiting_count = 0;
    //
    //    Serial.print("UID Length: ");
    //    Serial.print(uidLength, DEC);
    //    Serial.println(" bytes");
    //    Serial.print("UID Value: ");
    //    for (uint8_t i = 0; i < uidLength; i++)
    //    {
    //      nuidPICC[i] = uid[i];
    //      Serial.print(" "); Serial.print(uid[i], DEC);
    //    }
    //    Serial.println();
    //    tagId = tagToString(nuidPICC);
    //    dispTag = tagId;
    //    Serial.print(F("tagId is : "));
    //    Serial.println(tagId);
    //
    //    Serial.print("Type: "); Serial.print(detectType(tagId));
    //    if(last_UID != tagId){
    //      last_UID = tagId;
    //    }
    //    Serial.println("");
    NfcTag tag = nfc.read(); //reading the NFC card or tag

//    if ( last_card_UID == tag.getUidString() ) {
//      new_card = false;
//    } else {
      new_card = true;
//    }

    if (new_card) {
      last_card_UID = tag.getUidString();

      Serial.println("");
      Serial.print("Id: ");
      Serial.print(  tag.getUidString() );
      Serial.print(detectType(tag.getUidString()));
      Serial.println();

      delay(100);  // 1 second halt
    }

  }
  else
  {
    waiting = true;
    // PN532 probably timed out waiting for a card
  }

//  if (waiting) {
//    if (waiting_count < 50) {
//      waiting_count++;
//      Serial.print(".");
//    } else {
//      waiting_count = 0;
//      Serial.println("");
//    }
//  }

}

String tagToString(byte id[4])
{
  String tagId = "";
  for (byte i = 0; i < 4; i++)
  {
    if (i < 3) tagId += String(id[i]) + ".";
    else tagId += String(id[i]);
  }
  return tagId;
}



// DFPLAYER specific Functions---------------------


void pause()
{
  if (initSound == true) {
    myDFPlayer.playFolder(actual_folder, actual_track_n);
    Serial.println("-Playing first song- ");
    initSound = false;
  } else {
    myDFPlayer.pause();
    Serial.println("-Paused- ");
  }
  delay(500);
}

void resume()
{

  if (initSound == false && folder_changed == false) {
    myDFPlayer.resume();
    Serial.println("-Resumed- ");
  }

  if (initSound == true) {
    myDFPlayer.playFolder(actual_folder, actual_track_n);
    Serial.println("-Playing first song- ");
    initSound = false;
  }

  if (folder_changed == true) {
    myDFPlayer.playFolder(actual_folder, actual_track_n);
    Serial.println("-Playing first song- ");
    folder_changed = false;
  }

  delay(500);
}

void playNextSong() {

  if (initSound == true) {
    // Play init Sound
    actual_track_n = 1;
    initSound = false;
  } else {

    if (actual_folder != next_folder) {
      //after changing folder, play first sound of folder
      actual_track_n = 1;
      actual_folder = next_folder;
    } else {
      if (actual_track_n < num_tracks_in_folder) {
        actual_track_n++;
      } else {
        actual_track_n = 1;
      }

    }

  }

  myDFPlayer.playFolder(actual_folder, actual_track_n);
  //isPlaying = true;
  Serial.print("-Playing track ");
  Serial.print(actual_track_n);
  Serial.print("-");
  Serial.println();

  Serial.print("Track number: ");
  Serial.print(actual_track_n);
  Serial.print(" in folder :");
  Serial.print(actual_folder);
  Serial.println();

  delay(200);
}

void changeFolder() {

  if (next_folder < num_folders) {
    next_folder++;
  } else {
    next_folder = 1;
  }

  actual_track_n = 1;

  folder_changed  = true;

  delay(200);
}

void updateActualFolder() {
  actual_folder = next_folder;

  delay(200);
}



int checkForErrors() {
  int has_errors = 0;

  if ( !myDFPlayer.begin(mySoftwareSerial) ) {  //Use softwareSerial to communicate with mp3.
    Serial.println(F("Unable to begin:"));
    Serial.println(F("1.Please recheck the connection!"));
    Serial.println(F("2.Please insert the SD card!"));
    Serial.println(myDFPlayer.numSdTracks()); //read mp3 state

    has_errors = 1;

    while (true);
  }

  if ( myDFPlayer.numSdTracks() == -1) {
    has_errors = 1;
    has_media = false;
    Serial.println("- SD card not found");
  }

  return has_errors;
}

void turnOff() {
  Serial.println(F("TURNING OFF.."));
  isOn = false;

  myDFPlayer.playFolder(MP3_SOUNDS_FOLDER, 1); //Play the ON SOUND mp3
  //isPlaying = false;

}
