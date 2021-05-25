#include <Arduino.h>
#include <LiquidCrystal.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <LittleFS.h>
#include <AudioFileSourceLittleFS.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>
#include "config.h"

//AUDIO
AudioFileSourceLittleFS *file = nullptr;
AudioGeneratorMP3 *mp3 = nullptr;
AudioOutputI2S *out = nullptr;
char audioSource[256] = "";
const char* sounds[]={"/callboys5.mp3","/timer_demvanmij2.mp3","/timer_ieieie5.mp3","/timer_japper3.mp3","/timer_rarevibe3.mp3","/timer_sfeerspons3.mp3","/timer_situatie3.mp3"};

//TODO
// multiple songs + random
// change minutemillis to 3600*1000
// play song 3 times

//NTP
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

//LCD
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//TIMEKEEPING
unsigned long previousMillis = 0; 
bool timerConfig = false;
bool timerRunning = false;
bool initialSetup = true;
int timervalue = 1;
unsigned long lastTimerUpdateMillis = 0; 
unsigned long lastTimerCountdownMillis = 0; 
int currentTimerValue = 0;

void setup() {
  Serial.begin(115200);
  LittleFS.begin();
  setupLCD();
  setupWifi();
  setupNTP();
  Serial.println("Setup done, looping :)");
}

void loop() {
  //check wifi
  if (WiFi.status() != WL_CONNECTED) {
    setupWifi();
  }
    
  //check each hour how much days they are married
  unsigned long currentMillis = millis();
  if (initialSetup || (currentMillis - previousMillis >= interval && !timerConfig && !timerRunning)) {
    initialSetup = false;
    previousMillis = currentMillis;
    setDagenGetrouwd();
  }

  //check for button
  buttonState = digitalRead(buttonPin);
  if (buttonState == HIGH) {
    Serial.println("Button pressed");
    if (!timerConfig && !timerRunning){
      timerConfig=true;
      lastTimerUpdateMillis=currentMillis;
      timervalue = currentTimerValue+1;
      lcd.setCursor(0, 1);
      lcd.print("Timer: ");
      lcd.print(timervalue);
      lcd.print("min      ");
      delay(300);
    } else {
      lastTimerUpdateMillis=currentMillis;
      timervalue++;
      if (timervalue > maxtimer){
        timervalue=0;
      }
      lcd.setCursor(0, 1);
      lcd.print("Timer: ");
      lcd.print(timervalue);
      lcd.print("min      ");
      delay(300);
    }
  }
  //if timer configured for 5 seconds: start
  if (timerConfig && !timerRunning && currentMillis - lastTimerUpdateMillis >= timerStartInterval){
    Serial.println("Timer start");
    timerConfig = false;
    timerRunning = true;
    currentTimerValue = timervalue;
    lastTimerCountdownMillis = currentMillis;
  }

  if(timerRunning){
    if (currentMillis - lastTimerCountdownMillis >= minuteMillis){
      lastTimerCountdownMillis = currentMillis;
      currentTimerValue = currentTimerValue-1;
      if(currentTimerValue == 0){
        Serial.println("Timer done");
        playSound();
        //reset all
        timerConfig=false;
        timerRunning=false;
        timervalue=0;
        setDagenGetrouwd();
      }
    }
    lcd.setCursor(0, 1);
    lcd.print("Countdown: ");
    lcd.print(currentTimerValue);
    lcd.print("min      ");
    if(timervalue == 0){
      delay(500);
      setDagenGetrouwd();
    }
  }
}

void setupWifi(){
  lcd.setCursor(0, 1);
  lcd.print("Connecting to Wifi..");
  WiFi.mode(WIFI_STA);
  WiFi.hostname(ESP_NAME);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

void setupNTP(){
  timeClient.begin();
  Serial.println("NTP started");
}

void setupLCD(){
  lcd.begin(16, 2);
  lcd.print(" Jeroen & Sarah");
  lcd.setCursor(0, 1);
  Serial.println("LCD initialised");
}

void setDagenGetrouwd(){
  //get time and set days correctly
  timeClient.update();
  unsigned long t = timeClient.getEpochTime();
  int daysince1970 = t / 86400;
  int trouwdatum = 18775;
  int dagengetrouwd = daysince1970-trouwdatum;

  //write to lcd
  lcd.setCursor(0, 1);
  lcd.print(dagengetrouwd);
  lcd.print(" d getrouwd!    ");
}

void playSound(){
  const char* random_sound = sounds[random(7)];
  strlcpy(audioSource, random_sound, sizeof(audioSource));
  stopPlaying();
  if (!out) {
      out = new AudioOutputI2S();
      out->SetOutputModeMono(true);
  }
  out->SetGain(defaultGain);
  // Get MP3 from LittleFS
  Serial.printf_P(PSTR("**MP3 file: %s\n"), audioSource);
  file = new AudioFileSourceLittleFS(audioSource);
  mp3 = new AudioGeneratorMP3();

  mp3->begin(file, out);
  if (!mp3->isRunning()) {
      stopPlaying();
  }
  
  while (mp3->isRunning()) {
      if (!mp3->loop()) {
          stopPlaying();
      }
      yield();
  }
}

bool stopPlaying() {
    bool stopped = false;
    if (mp3) {
        mp3->stop();
        mp3 = nullptr;
        stopped = true;
    }
    if (file) {
        file->close();
        file = nullptr;
    }

    return stopped;
}
