// ********************************************************************
//  Copyright (c) 2022 weboo
//  This software is released under the MIT License, see LICENSE.
// ********************************************************************

#include <Arduino.h>

#include <HTTPClient.h>
#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>
#include <NTPClient.h>
#include <rpcWiFi.h>
#include <RTC_SAMD51.h>
#include <SAMCrashMonitor.h>
#include <Time.h>

#define JD_FORMAT      1    // 0:RGB888, 1:RGB565
#define LCD_WIDTH      320  // LCDの横ドット数
#define LCD_HEIGHT     240  // LCDの縦ドット数
#define LCD_BRIGHTNESS 128  // バックライトの明るさ(0-255)

const char* ssid     = "*****";   // Wi-Fi接続のアクセスポイント名
const char* password = "*****";   // Wi-Fi接続のパスワード

RTC_SAMD51 rtc;
WiFiUDP udp;
NTPClient timeClient(udp, "ntp.jst.mfeed.ad.jp");

static LGFX lcd;

static const char* urlFormat = "http://www.bijint.com/assets/pict/tokyo/pc/%.2d%.2d.jpg";
char imgUrl[64];
static int lastMinute = -1;


void playTone(int tone, int duration)
{
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    analogWrite(WIO_BUZZER, 128);
    delayMicroseconds(tone);
    analogWrite(WIO_BUZZER, 0);
    delayMicroseconds(tone);
  }
}


void adjustTime()
{
  lcd.println(F("Sync NTP Server"));

  timeClient.begin();
  timeClient.setTimeOffset(9 * 60 * 60);  // sec
  timeClient.update();

  if (!rtc.begin()) {
    lcd.println(F("Couldn't find RTC"));
    while (1) delay(10);
  }

  rtc.adjust(timeClient.getEpochTime());

  lcd.print(F("Time: "));
  lcd.println(rtc.now().timestamp(DateTime::TIMESTAMP_FULL));
}


void setup() {
  Serial.begin(115200);

  pinMode(WIO_BUZZER, OUTPUT);

  lcd.init();
  lcd.setRotation(1);
  lcd.setTextWrap(true, true);
  lcd.setBrightness(LCD_BRIGHTNESS);
  lcd.fillScreen(TFT_BLACK);
  lcd.setTextColor(TFT_WHITE);
  lcd.setFont(&fonts::Font4);

  WiFi.mode(WIFI_STA);
  // WiFi.disconnect();

  lcd.print(F("WiFi: "));
  lcd.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(300);
      WiFi.begin(ssid, password);
      lcd.print(".");
  }
  lcd.print(F("\nIP Address: "));
  lcd.println (WiFi.localIP());

  // NTP時刻更新
  adjustTime();

  playTone(440, 50);
  delay(1000);

  SAMCrashMonitor::begin();
  SAMCrashMonitor::disableWatchdog();
  SAMCrashMonitor::dump();
  SAMCrashMonitor::enableWatchdog(30 * 1000);

  lcd.setTextWrap(false);
  lcd.setFont(&fonts::Font6);
  lcd.fillScreen(TFT_BLACK);
}


void loop() {
  SAMCrashMonitor::iAmAlive();

  DateTime now = rtc.now();
  if (now.minute() != lastMinute) {
    sprintf(imgUrl, urlFormat, now.hour(), now.minute());

    bool drawResult = lcd.drawJpgUrl(imgUrl, 13, 8, LCD_WIDTH, LCD_HEIGHT, 0, 0, 0.5);
    if (drawResult) {
      lastMinute = now.minute();
    } else if (now.second() > 10) {
      lcd.fillRect(13, 8, 295, 225, TFT_BLUE);
      lcd.setTextColor(TFT_WHITE);
      lcd.setCursor(10, 10);
      lcd.printf("%.2d:%.2d", now.hour(), now.minute());
    }
  }

  delay(1000);
}
