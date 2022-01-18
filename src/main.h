#pragma once
#ifndef _main_h
#define _main_h
// LIBRARY
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <LiquidCrystal_I2C.h>
#include <NTPClient.h>
#include <Servo.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <addons/RTDBHelper.h>
#include <addons/TokenHelper.h>

#include "CTBot.h"

// WIFI
#define WIFI_SSID "paiko-iot"
#define WIFI_PASSWORD "1sampai8"

// SETTING FIREBASE
#define API_KEY "AIzaSyAof0qcjxxC0ofkPm7fBsl4Ay8bZEeCoIM"
#define DATABASE_URL \
    "https://paiko-87e4b-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "vigosikoboy@gmail.com"
#define USER_PASSWORD "airmineral123"

// NTP CLIENT -> Mengambil waktu dari internet
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 25200, 60000);

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Telegram bot
#define TOKEN "5097989646:AAF1jInUFpb4pqOndGBXnN4cPJwUWPcb9h4"
#define ID 632206286
CTBot myBot;

// PIN - PIN
#define servoPin D4
#define btnPin D0
LiquidCrystal_I2C lcd(0x27, 16, 4);
Servo myservo;

// VAR LAIN
TBMessage msg;
TBUser user;
String weekDays[7]={"Minggu", "Senin", "Selasa", "Rabu", "Kamis", "Jumat", "Sabtu"};
bool btnState, botState, alarmState = 1;
int setTimeState;
int lastTimeBotRan;
const int bot_delay = 1000;
String alarm1, alarm2;
int hr1, hr2, min1, min2;

void handleNewMessages(String hari, String tanggal, String waktu);
void pushHistory(String hari, String tanggal, String waktu);
void feedFish();
bool validateTime(String waktu);
void handleAlarm(int hour, int minute, String hari, String tanggal, String waktu);
#endif