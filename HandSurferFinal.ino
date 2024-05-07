// ~~~~~~~~~~ Universal Libraries ~~~~~~~~~~
#include <SPI.h> //Serial peripheral interface library
#include "Arduino.h"

// ~~~~~~~~~~ Sonar Range Finder Libraries ~~~~~~~~~~
#include "SonarEZ0pw.h" //Sonar library
SonarEZ0pw sonarL(7); //Connection pin
SonarEZ0pw sonarM(8); //Connection pin
SonarEZ0pw sonarR(9); //Connection pin
float Ldis = 0;  //in CM
float Mdis = 0;
float Rdis = 0;

// ~~~~~~~~~~ LED Section ~~~~~~~~~~
#include <FastLED.h>  //LED strip library
#define NUM_LEDS 145  //Declare number of used LEDs
#define LED_PIN 2 //Connection pin
CRGB leds[NUM_LEDS]; //declare LED array
const CRGB onLED = CRGB::White;
const CRGB hitLED = CRGB::Green;
const CRGB missLED = CRGB::Red;
const CRGB offLED = CRGB::Black;

// ~~~~~~~~~~ Audio Player Section ~~~~~~~~~~
#include "DFRobotDFPlayerMini.h" //Audio player library
#include <SoftwareSerial.h> //Software serial library
SoftwareSerial mySerial(10, 11); //Instantiate pins
DFRobotDFPlayerMini myDFPlayer; //Instantiate player

// ~~~~~~~~~~ Dot Matrix Display Section ~~~~~~~~~~
#include <MD_Parola.h> //animated display library
#include <MD_MAX72xx.h> //dot matrix library
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN 3
#define DATA_PIN 4
#define CLK_PIN 5
MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES); //Instantiate special text display

// ~~~~~~~~~~ Game specific values ~~~~~~~~~~
unsigned long startMillis; 
unsigned long currentMillis;
int songSelection;  //currently selected song
bool inMenu;  //bool to check if in game state or menu state

float BPM;  //NOT ACTUAL BPM, but rather ms per 16th note
int measure;
int note;

int handRegion;

bool handRegionL; bool handRegionM; bool handRegionR;

int score;
int lBuffer; int mBuffer; int rBuffer; //incremented to ensure no erroneous sensor readings


void (*resetFunc) (void) = 0;

void setup(){
  Serial.begin(9600);
  startMillis = millis();  //initial start time
  songSelection = 1;
  
  //setup Audio player
  mySerial.begin(9600);
  if(myDFPlayer.begin(mySerial)){
    Serial.println("Audio Player Success");
    myDFPlayer.volume(24);  //0-30
  }
  else{
    Serial.println("Audio Player Fail");
  }
  
  //setup LED strip
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
  
  //setup Dot matrix display
  myDisplay.begin();
	myDisplay.setIntensity(2);
	myDisplay.displayClear();
  
	inMenu = true;

	BPM = 125;
	handRegion = 0;
  handRegionL = false; handRegionM = false; handRegionR = false; 
	score = 0;
  lBuffer = 0; mBuffer = 0; rBuffer = 0; 

	myDisplay.displayScroll("Metal Blues", PA_LEFT, PA_SCROLL_LEFT, 50);
}

void loop(){
  Ldis = sonarL.Distance(cm);
  Mdis = sonarM.Distance(cm);
  Rdis = sonarR.Distance(cm);

  //menu interface
  if(inMenu){
    MENUdetectPosition(Ldis, Mdis, Rdis);
/*
    if(handRegion == 1){ //selecting left
      songRotation(false);
      delay(1000);
    }
    if(handRegion == 3){ //selecting right
      songRotation(true);
      delay(1000);
    }*/
    
    if(handRegion == 2){ //selecting middle
      inMenu = false;
      myDisplay.displayClear();
      myDisplay.setTextAlignment(PA_CENTER);
	    myDisplay.print("Start!");
      measure = 1;
	    note = 7;
      delay(2000);  //"just in case" delay
      startMillis = millis();
      myDFPlayer.play(1);
    }
    
    if (myDisplay.displayAnimate()) {
      myDisplay.displayReset();
    }
  }
  
  //rhythm game portion
  else{
    detectPosition(Ldis, Mdis, Rdis);
    currentMillis = millis();
    if(currentMillis - startMillis > BPM){ //checks everytime 1 16th note occurs
      startMillis = startMillis + BPM;
      if(note < 16){  //incriment note every 125ms
        note++;
      }
      else{
        note = 1;
        measure++;
      }
      updateLights();
      metalBlues(note, measure);
      FastLED.show();
    }
    collisionCheck();
    myDisplay.print(score);
  }
}

void MENUdetectPosition(float Ldis, float Mdis, float Rdis){
  if(Ldis > 12 && Ldis < 30){
    if(lBuffer < 5){
      lBuffer++;
    }
    else{
      handRegion = 1;
    }
  }
  else if(Mdis > 12 && Mdis < 30){
    if(mBuffer < 5){
      mBuffer++;
    }
    else{
      handRegion = 2;
    }
  }
  else if(Rdis > 12 && Rdis < 30){
    if(rBuffer < 5){
      rBuffer++;
    }
    else{
      handRegion = 3;
    }
  }
  else{
    lBuffer = 0; mBuffer = 0; rBuffer = 0;
    handRegion = 0;
  }
}

void detectPosition(float Ldis, float Mdis, float Rdis){
  if(Ldis > 12 && Ldis < 30){
    handRegionL = true; 
  }
  if(Mdis > 12 && Mdis < 30){
    handRegionM = true; 
  }
  if(Rdis > 12 && Rdis < 30){
    handRegionR = true; 
  }
  if(Ldis > 30){
    handRegionL = false;
  }
  if(Mdis > 30){
    handRegionM = false;
  }
  if(Rdis > 30){
    handRegionR = false; 
  }
}

void updateLights(){
  if(leds[1] == hitLED || leds[1] == missLED){
    for(int i = 1; i < 7; i++){
      leds[i] = offLED;
    }
  }
  else if(leds[1] == onLED){
    for(int i = 1; i < 7; i++){
      leds[i] = missLED;
    }
  }

  if(leds[7] == hitLED || leds[7] == missLED){
    for(int i = 7; i < 13; i++){
      leds[i] = offLED;
    }
  }
  else if(leds[7] == onLED){
    for(int i = 7; i < 13; i++){
      leds[i] = missLED;
    }
  }

  if(leds[13] == hitLED || leds[13] == missLED){
    for(int i = 13; i < 19; i++){
      leds[i] = offLED;
    }
  }
  else if(leds[13] == onLED){
    for(int i = 13; i < 19; i++){
      leds[i] = missLED;
    }
  }

  for(int i = 19; i < 145; i++){
    if(leds[i] == onLED){
      leds[i] = offLED;
      leds[i-18] = onLED;
    }
  }
}

void spawnLight(int column){
  if(column == 1){
    for(int i = 127; i < 133; i++){
      leds[i] = onLED;
    }
  }
  else if(column == 2){
    for(int i = 133; i < 139; i++){
      leds[i] = onLED;
    }
  }
  else if(column == 3){
    for(int i = 139; i < 145; i++){
      leds[i] = onLED;
    }
  }
}

void collisionCheck(){
  if(handRegionL && leds[3] == onLED){
    score = score + 100;
    for(int i = 1; i < 7; i++){
      leds[i] = hitLED;
    }
    FastLED.show();
  }
  if(handRegionM && leds[9] == onLED){
    score = score + 100;
    for(int i = 7; i < 13; i++){
      leds[i] = hitLED;
    }
    FastLED.show();
  }
  if(handRegionR && leds[15] == onLED){
    score = score + 100;
    for(int i = 13; i < 19; i++){
      leds[i] = hitLED;
    }
    FastLED.show();
  }
}

void songRotation(bool dir){
  myDisplay.displayClear();
  if(dir){  //scroll right
    if(songSelection == 1){
      songSelection = 2;
    }
    else if(songSelection == 2){
      songSelection = 3;
    }
    else if(songSelection == 3){
      songSelection = 1;
    }
  }
  else{ //scroll left
    if(songSelection == 1){
      songSelection = 3;
    }
    else if(songSelection == 2){
      songSelection = 1;
    }
    else if(songSelection == 3){
      songSelection = 2;
    }
  }

  if(songSelection == 1){
    BPM = 125;
    myDisplay.displayScroll("Metal Blues", PA_LEFT, PA_SCROLL_LEFT, 25);
  }
  if(songSelection == 2){
    BPM = 150;
    myDisplay.displayScroll("10000", PA_LEFT, PA_SCROLL_LEFT, 25);
  }
  if(songSelection == 3){
    BPM = 75;
    myDisplay.displayScroll("Turbo Strawberry", PA_LEFT, PA_SCROLL_LEFT, 25);
  }
}

//Song 1: "Metal Blues", 120BPM, 125ms/16th note, 2000ms/measure (medium speed)
void metalBlues(int note, int measaure){
  if(measure == 5){
    if(note == 1){
      spawnLight(1);
    }
    if(note == 5){
      spawnLight(1);
    }
    if(note == 9){
      spawnLight(1);
    }
    if(note == 13){
      spawnLight(1);
    }
  }
  
  else if(measure == 6){
    if(note == 1){
      spawnLight(3);
    }
    if(note == 5){
      spawnLight(3);
    }
    if(note == 9){
      spawnLight(3);
    }
    if(note == 13){
      spawnLight(3);
    }
  }
  
  else if(measure == 7){
    if(note == 1){
      spawnLight(1);
    }
    if(note == 5){
      spawnLight(1);
    }
    if(note == 9){
      spawnLight(1);
    }
    if(note == 13){
      spawnLight(1);
    }
  }
  
  else if(measure == 8){
    if(note == 1){
      spawnLight(2);
    }
    if(note == 5){
      spawnLight(2);
    }
    if(note == 9){
      spawnLight(2);
    }
    if(note == 13){
      spawnLight(2);
    }
  }
  
  else if(measure == 9){
    if(note == 1){
      spawnLight(1);
    }
    if(note == 5){
      spawnLight(2);
    }
    if(note == 9){
      spawnLight(2);
    }
    if(note == 13){
      spawnLight(3);
    }
  }
  
  else if(measure == 10){
    if(note == 1){
      spawnLight(3);
    }
    if(note == 5){
      spawnLight(2);
    }
    if(note == 9){
      spawnLight(2);
    }
    if(note == 13){
      spawnLight(1);
    } 
  }
  
  else if(measure == 11){
    if(note == 1){
      spawnLight(2);
    }
    if(note == 5){
      spawnLight(3);
    }
    if(note == 9){
      spawnLight(2);
    }
    if(note == 13){
      spawnLight(1);
    } 
  }
  
  else if(measure == 12){
    if(note == 1){
      spawnLight(1);
    }
    if(note == 3){
      spawnLight(1);
    }
    if(note == 5){
      spawnLight(1);
    }
    if(note == 9){
      spawnLight(2);
    }
    if(note == 11){
      spawnLight(2);
    }
    if(note == 13){
      spawnLight(2);
    } 
  }
  
  else if(measure == 13){
    if(note == 1){
      spawnLight(2);
    }
    if(note == 2){
      spawnLight(2);
    }
    if(note == 4){
      spawnLight(2);
    }
    if(note == 5){
      spawnLight(2);
    }
    if(note == 9){
      spawnLight(3);
    }
    if(note == 13){
      spawnLight(3);
    }
  }
  
  else if(measure == 14){
    if(note == 1){
      spawnLight(2);
    }
    if(note == 2){
      spawnLight(2);
    }
    if(note == 4){
      spawnLight(2);
    }
    if(note == 5){
      spawnLight(2);
    }
    if(note == 9){
      spawnLight(1);
    }
    if(note == 13){
      spawnLight(1);
    }
  }
  
  else if(measure == 15){
    if(note == 1){
      spawnLight(1);
    }
    if(note == 2){
      spawnLight(1);
    }
    if(note == 4){
      spawnLight(2);
    }
    if(note == 5){
      spawnLight(2);
    }
    if(note == 9){
      spawnLight(3);
    }
    if(note == 13){
      spawnLight(3);
    }
  }
  
  else if(measure == 16){
    if(note == 1){
      spawnLight(3);
    }
    if(note == 3){
      spawnLight(3);
    }
    if(note == 5){
      spawnLight(2);
    }
    if(note == 7){
      spawnLight(2);
    }
    if(note == 9){
      spawnLight(1);
    }
    if(note == 11){
      spawnLight(1);
    }
    if(note == 13){
      spawnLight(1);
    }
    if(note == 15){
      spawnLight(1);
    }
  }
  
  else if(measure == 17){
    if(note == 1){
      spawnLight(1);
    }
    if(note == 2){
      spawnLight(1);
    }
    if(note == 4){
      spawnLight(1);
    }
    if(note == 5){
      spawnLight(1);
    }
    if(note == 9){
      spawnLight(2);
    }
    if(note == 13){
      spawnLight(2);
    }
  }
  
  else if(measure == 18){
    if(note == 1){
      spawnLight(3);
    }
    if(note == 2){
      spawnLight(3);
    }
    if(note == 4){
      spawnLight(3);
    }
    if(note == 5){
      spawnLight(3);
    }
    if(note == 9){
      spawnLight(2);
    }
    if(note == 13){
      spawnLight(2);
    }
  }
  
  else if(measure == 19){
    if(note == 1){
      spawnLight(1);
    }
    if(note == 2){
      spawnLight(1);
    }
    if(note == 4){
      spawnLight(1);
    }
    if(note == 5){
      spawnLight(1);
    }
    if(note == 9){
      spawnLight(2);
    }
    if(note == 13){
      spawnLight(2);
    }
  }
  
  else if(measure == 20){
    if(note == 1){
      spawnLight(1);
    }
    if(note == 3){
      spawnLight(1);
    }
    if(note == 5){
      spawnLight(2);
    }
    if(note == 7){
      spawnLight(2);
    }
    if(note == 9){
      spawnLight(3);
    }
    if(note == 11){
      spawnLight(3);
    }
    if(note == 13){
      spawnLight(3);
    }
    if(note == 15){
      spawnLight(3);
    }
  }
  
  else if(measure == 21){
    if(note == 1){
      spawnLight(3);
    }
    if(note == 2){
      spawnLight(3);
    }
    if(note == 3){
      spawnLight(3);
    }
    if(note == 4){
      spawnLight(3);
    }
    if(note == 5){
      spawnLight(3);
    }
    if(note == 6){
      spawnLight(3);
    }
    if(note == 7){
      spawnLight(3);
    }
    if(note == 8){
      spawnLight(3);
    }
    if(note == 9){
      spawnLight(3);
    }
    if(note == 10){
      spawnLight(3);
    }
    if(note == 11){
      spawnLight(3);
    }
    if(note == 12){
      spawnLight(3);
    }
    if(note == 13){
      spawnLight(3);
    }
    if(note == 14){
      spawnLight(3);
    }
  }
  
  else if(measure == 22){
    if(note == 1){
      spawnLight(2);
    }
    if(note == 2){
      spawnLight(2);
    }
    if(note == 3){
      spawnLight(2);
    }
    if(note == 4){
      spawnLight(2);
    }
    if(note == 5){
      spawnLight(2);
    }
    if(note == 6){
      spawnLight(2);
    }
    if(note == 7){
      spawnLight(2);
    }
    if(note == 8){
      spawnLight(2);
    }
    if(note == 9){
      spawnLight(2);
    }
    if(note == 10){
      spawnLight(2);
    }
    if(note == 11){
      spawnLight(2);
    }
    if(note == 12){
      spawnLight(2);
    }
    if(note == 13){
      spawnLight(2);
    }
    if(note == 14){
      spawnLight(2);
    }
  }
  
  else if(measure == 23){
    if(note == 1){
      spawnLight(3);
    }
    if(note == 5){
      spawnLight(2);
    }
    if(note == 9){
      spawnLight(1);
    }
    if(note == 13){
      spawnLight(2);
    }
  }
  
  else if(measure == 24){
    if(note == 1){
      spawnLight(1);
    }
    if(note == 5){
      spawnLight(2);
    }
    if(note == 9){
      spawnLight(3);
    }
    if(note == 13){
      spawnLight(2);
    }
  }
  
  else if(measure == 25){
    if(note == 1){
      spawnLight(3);
    }
    if(note == 2){
      spawnLight(3);
    }
    if(note == 3){
      spawnLight(3);
    }
    if(note == 4){
      spawnLight(3);
    }
    if(note == 5){
      spawnLight(3);
    }
    if(note == 6){
      spawnLight(3);
    }
    if(note == 7){
      spawnLight(3);
    }
    if(note == 8){
      spawnLight(3);
    }
    if(note == 9){
      spawnLight(3);
    }
    if(note == 10){
      spawnLight(3);
    }
    if(note == 11){
      spawnLight(3);
    }
    if(note == 12){
      spawnLight(3);
    }
    if(note == 13){
      spawnLight(3);
    }
    if(note == 14){
      spawnLight(3);
    }
  }
  
  else if(measure == 26){
    if(note == 1){
      spawnLight(2);
    }
    if(note == 2){
      spawnLight(2);
    }
    if(note == 3){
      spawnLight(2);
    }
    if(note == 4){
      spawnLight(2);
    }
    if(note == 5){
      spawnLight(2);
    }
    if(note == 6){
      spawnLight(2);
    }
    if(note == 7){
      spawnLight(2);
    }
    if(note == 8){
      spawnLight(2);
    }
    if(note == 9){
      spawnLight(2);
    }
    if(note == 10){
      spawnLight(2);
    }
    if(note == 11){
      spawnLight(2);
    }
    if(note == 12){
      spawnLight(2);
    }
    if(note == 13){
      spawnLight(2);
    }
    if(note == 14){
      spawnLight(2);
    }
  }
  
  else if(measure == 27){
    if(note == 1){
      spawnLight(3);
    }
    if(note == 5){
      spawnLight(3);
    }
    if(note == 9){
      spawnLight(1);
    }
    if(note == 13){
      spawnLight(1);
    }
  }
  
  else if(measure == 28){
    if(note == 1){
      spawnLight(1);
    }
    if(note == 5){
      spawnLight(1);
    }
    if(note == 9){
      spawnLight(3);
    }
    if(note == 13){
      spawnLight(3);
    }
  }
  
  else if(measure == 29){
    if(note == 1){
      spawnLight(2);
    }
    if(note == 2){
      spawnLight(2);
    }
    if(note == 3){
      spawnLight(2);
    }
    if(note == 4){
      spawnLight(2);
    }
    if(note == 5){
      spawnLight(2);
    }
    if(note == 6){
      spawnLight(2);
    }
    if(note == 7){
      spawnLight(2);
    }
    if(note == 8){
      spawnLight(2);
    }
    if(note == 9){
      spawnLight(2);
    }
    if(note == 10){
      spawnLight(2);
    }
    if(note == 11){
      spawnLight(2);
    }
    if(note == 12){
      spawnLight(2);
    }
    if(note == 13){
      spawnLight(2);
    }
    if(note == 14){
      spawnLight(2);
    }
    if(note == 15){
      spawnLight(2);
    }
    if(note == 16){
      spawnLight(2);
    }
  }
  
  else if(measure == 30){
    if(note == 1){
      spawnLight(2);
    }
    if(note == 2){
      spawnLight(2);
    }
    if(note == 3){
      spawnLight(2);
    }
    if(note == 4){
      spawnLight(2);
    }
    if(note == 5){
      spawnLight(2);
    }
    if(note == 6){
      spawnLight(2);
    }
    if(note == 7){
      spawnLight(2);
    }
    if(note == 8){
      spawnLight(2);
    }
    if(note == 9){
      spawnLight(2);
    }
    if(note == 10){
      spawnLight(2);
    }
    if(note == 11){
      spawnLight(2);
    }
    if(note == 12){
      spawnLight(2);
    }
    if(note == 13){
      spawnLight(2);
    }
    if(note == 14){
      spawnLight(2);
    }
    if(note == 15){
      spawnLight(2);
    }
    if(note == 16){
      spawnLight(2);
    }
  }
  
  else if(measure == 31){
    if(note == 1){
      spawnLight(2);
    }
    if(note == 2){
      spawnLight(2);
    }
    if(note == 3){
      spawnLight(2);
    }
    if(note == 4){
      spawnLight(2);
    }
    if(note == 5){
      spawnLight(2);
    }
    if(note == 6){
      spawnLight(2);
    }
    if(note == 7){
      spawnLight(2);
    }
    if(note == 8){
      spawnLight(2);
    }
    if(note == 9){
      spawnLight(2);
    }
    if(note == 10){
      spawnLight(2);
    }
    if(note == 11){
      spawnLight(2);
    }
    if(note == 12){
      spawnLight(2);
    }
    if(note == 13){
      spawnLight(2);
    }
    if(note == 14){
      spawnLight(2);
    }
    if(note == 15){
      spawnLight(2);
    }
    if(note == 16){
      spawnLight(2);
    }
  }
    
  //finish the song and go back to the menu
  else if(measure == 34){
    inMenu = true;
    score = 0;
    myDFPlayer.pause();
    myDisplay.begin();
    myDisplay.setIntensity(2);
    myDisplay.displayClear();
    myDisplay.displayScroll("Metal Blues", PA_LEFT, PA_SCROLL_LEFT, 50);

    resetFunc();
  }
}