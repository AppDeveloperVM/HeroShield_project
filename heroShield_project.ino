#include <DFPlayerMini_Fast.h>
#include <ezButton.h>
#include "Arduino.h"
#include "SoftwareSerial.h"

//including the library for I2C communication 
#include <Wire.h>
//including the library for I2C communication with the PN532 module, library for the PN532 module and the library for the NFC
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>

#define rxPin 10
#define txPin 11

#define LED_A 3
#define LED_B 5

#define POWER_BUTTON 6

//ezButton POWER_BUTTON(6);
ezButton CHANGE_FOLDER(7);
ezButton NEXT_BUTTON(8);
ezButton PAUSE_BUTTON(9);

#define VLM_PIN A0
#define SAMPLES 10 //average of x values to stabilize reading

#define BUSY_PIN 2

#define VOLUME_LEVEL 16 // 0 - 30 ( 18 is a good level )
#define MAX_VLM_LVL 17
#define MP3_SOUNDS_FOLDER 10

int fadeDuration = 800; //LEDS fade duration in ms

int num_tracks_in_folder = 0;
int num_folders = 2;
int actual_track_n = 0;
int actual_folder = 1;
int next_folder = 1;

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
  
  pinMode(LED_A, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(BUSY_PIN,INPUT);
  pinMode(VLM_PIN,INPUT_PULLUP);
  pinMode(POWER_BUTTON,INPUT_PULLUP);

  //POWER_BUTTON.setDebounceTime(50);
  CHANGE_FOLDER.setDebounceTime(20);
  NEXT_BUTTON.setDebounceTime(50);
  PAUSE_BUTTON.setDebounceTime(50);

  current_volume = VOLUME_LEVEL;
  
  mySoftwareSerial.begin(9600);
  Serial.begin(115200);

  //Starting
  Serial.println();
  Serial.println(F("DFRobot DFPlayer Mini"));
  Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));
  
  
  Serial.println(F("DFPlayer Mini online."));
  // check errors+
  if( checkForErrors() != 1 ){
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
  }else{
    Serial.println("[ Some errors to fix ]");
  }
  
  
  
}

void loop()
{
  
  //POWER_BUTTON.loop();
  CHANGE_FOLDER.loop();
  NEXT_BUTTON.loop();
  PAUSE_BUTTON.loop();

  if(digitalRead(BUSY_PIN) == LOW ){
    isPlaying = true;
  }else if( digitalRead(BUSY_PIN) == HIGH ) {
    isPlaying = false;
  }

  // POTENTIOMETER - VOLUME 
  if(isOn){
    
  
    for (int i=0; i< SAMPLES ; i++){
      inputVolume += analogRead(VLM_PIN);  //Volume lvl recived by Potentiometer
    }
    
    inputVolume /= SAMPLES ;
    
    outputVolume = map(inputVolume, 0, 1023, 0, MAX_VLM_LVL);
  
    if( (outputVolume != current_volume) && (outputVolume <= MAX_VLM_LVL) ){
       // modify actual volume output
      current_volume = outputVolume;
      
      if(current_volume <= MAX_VLM_LVL){
        myDFPlayer.volume(current_volume);
  
        Serial.println();
        Serial.print("Volume changed to: ");
        Serial.print(current_volume);
        Serial.println();
      }
      
    }

  }

  // POWER BUTTON
  powerState = digitalRead(POWER_BUTTON);
  if(powerState == true){ // it has a value
    powerStatus = !powerStatus;
      
        if (powerStatus == On)
        {
          Initiation();
        }
        else if(powerStatus == Off)
        {
          turnOff();
        }
        lastPowerState = powerState;
    
  } while(digitalRead(POWER_BUTTON) == true);
  delay(50);
  

  if(PAUSE_BUTTON.isPressed() && PAUSE_BUTTON.getStateRaw() == LOW && powerStatus == On)
  {
    if(isPlaying)
    {
      pause();
      //isPlaying = false;  
    }
    else
    {
      resume();
    }
    
  }


  if(NEXT_BUTTON.isPressed() && NEXT_BUTTON.getStateRaw() == LOW && powerStatus == On)
  {
    playNextSong();
  }

  if(CHANGE_FOLDER.isPressed() && CHANGE_FOLDER.getStateRaw() == LOW && powerStatus == On)
  {
    
    changeFolder();
    //'next_folder' value changed
    updateActualFolder();
    
    Serial.println();
    Serial.print("Changing folder to: ");
    Serial.print(next_folder);
    Serial.println();

    num_tracks_in_folder = myDFPlayer.numTracksInFolder(next_folder);
    Serial.print("Tracks in folder ");
    Serial.print(next_folder);
    Serial.print(": ");
    Serial.print(num_tracks_in_folder);
    Serial.println();
    
  }

  

}

void Initiation(){
  // play FURRRÑIUUNN microphone sound
  Serial.println();
  Serial.println(F("STARTING.."));
  isOn = true;
  
  myDFPlayer.playFolder(MP3_SOUNDS_FOLDER,1);  //Play the ON SOUND mp3
  actual_track_n = 1;
  initSound = true;
  delay(200);
  fadeLed(digitalRead(LED_A));
}

void fadeLed(boolean input){
  for(int state=0;state<256;state++){
    if (input==LOW){
      analogWrite(LED_A, state);
      analogWrite(LED_B, state);
    }else{
      analogWrite(LED_A, 255-state);
      analogWrite(LED_B, 255-state);
    }
    delay(fadeDuration/256); //=(total fading duration)/(number of iterations)
  }
}



void setup_rgb(){
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(10, OUTPUT);
}

String detectType(String UID){
  type = "Not recognized";
  if(UID == "180.103.200.115"){
    type = "red";
    Color(255 ,0 ,0);
  }
  if(UID == "195.240.72.146"){
    type = "blue";
    Color(0 ,0, 255);
  }
  if(UID == "99.28.84.167"){
    type = "reset";
    Color(0 ,0, 0);
  }
  return type;
}



// LED functions ---------------------------------

void Color(int R, int G, int B){     
      digitalWrite(10 , R) ;   // Red    - Rojo
      digitalWrite(9, G) ;   // Green - Verde
      digitalWrite(8, B) ;   // Blue - Azul
}




// NFC specific Functions ------------------------

void readNFC()
{
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                       // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  if (success)
  {
    Serial.println("");
    waiting = false;
    waiting_count = 0;
    
    Serial.print("UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i = 0; i < uidLength; i++)
    {
      nuidPICC[i] = uid[i];
      Serial.print(" "); Serial.print(uid[i], DEC);
    }
    Serial.println();
    tagId = tagToString(nuidPICC);
    dispTag = tagId;
    Serial.print(F("tagId is : "));
    Serial.println(tagId);
    
    Serial.print("Type: "); Serial.print(detectType(tagId));
    if(last_UID != tagId){
      changeLed();
      last_UID = tagId;
    }
    
    Serial.println("");

    
    delay(1000);  // 1 second halt
  }
  else
  {
    waiting = true;
    // PN532 probably timed out waiting for a card
  }

  if(waiting){
    if(waiting_count < 100){
      waiting_count++;
      Serial.print(".");
    }else{
      waiting_count = 0;
      Serial.println("");
    }
  }
  
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
  if(initSound == true){
    myDFPlayer.playFolder(actual_folder,actual_track_n);
    Serial.println("-Playing first song- ");
    initSound = false;
  }else{
    myDFPlayer.pause();
    Serial.println("-Paused- ");
  }
  delay(500);
}

void resume()
{

  if(initSound == false && folder_changed == false){
    myDFPlayer.resume();
    Serial.println("-Resumed- ");
  }
  
  if(initSound == true){
    myDFPlayer.playFolder(actual_folder,actual_track_n);
    Serial.println("-Playing first song- ");
    initSound = false;
  }

  if(folder_changed == true){
    myDFPlayer.playFolder(actual_folder,actual_track_n);
    Serial.println("-Playing first song- ");
    folder_changed = false;
  }
  
  delay(500);
}

void playNextSong(){

  if(initSound == true){
    // Play init Sound
    actual_track_n = 1;
    initSound = false;
  }else{

    if(actual_folder != next_folder){
      //after changing folder, play first sound of folder
       actual_track_n = 1;
       actual_folder = next_folder;
    }else{
      if(actual_track_n < num_tracks_in_folder) {
        actual_track_n++;
      }else{
        actual_track_n = 1;
      }
        
    }
      
  }

  myDFPlayer.playFolder(actual_folder,actual_track_n);
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

void changeFolder(){

  if(next_folder < num_folders){
    next_folder++;
  }else{
    next_folder = 1;
  }

  actual_track_n = 1;
  
  folder_changed = true;
  
  delay(200);
}

void updateActualFolder(){
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
    
    while(true);
  }

  if( myDFPlayer.numSdTracks() == -1){
    has_errors = 1;
    has_media = false;
    Serial.println("- SD card not found");
  }

  return has_errors;
}

void turnOff(){
  Serial.println(F("TURNING OFF.."));
  isOn = false;
  
  myDFPlayer.playFolder(MP3_SOUNDS_FOLDER,1);  //Play the ON SOUND mp3
  //isPlaying = false;
  fadeLed(digitalRead(LED_A));
}



 
