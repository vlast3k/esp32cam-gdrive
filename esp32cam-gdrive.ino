#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "Base64.h"
#include <HTTPClient.h>
#include "esp_camera.h"
#include "driver/rtc_io.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc.h"

const char* ssid     = "vladiVivacom4g";   //your network SSID
const char* password = "0888414447";   //your network password
const char* myDomain = "script.google.com";
String myScript = "/macros/s/AKfycbyawOlBvc9I-XmiTRZZvA6ZnhROP3LRAjx1jOcrd1xw_iRB7cU/exec";    //Replace with your own url

int waitingTime = 30000; //Wait 30 seconds to google response.
String timesForImage [] = { "09:00", "11:00", "13:00", "15:00", "17:00", ""};

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  Serial.begin(115200);
  delay(10);

  //Serial.printf("6: %d, 2: %d, 4: %d \n", digitalRead(6));
  initWifi();

  //testTimeToSleep();
  //delay(1000000L);
  
  initCamera();
  Serial.println(F("retrieve time from google")); 
  uint32_t secToSleep = getSecondsToSleep(getTimeFromGoogleSec() + 2*3600);
  if (secToSleep > 0) {
    Serial.printf("Will sleep for %u sec, until next interval\n", secToSleep);
    saveCapturedImage();
    enterDeepSleep(secToSleep);
  }
  saveCapturedImage();
  enterDeepSleep(120);
}

void enterDeepSleep(uint32_t secToSleep) {
  esp_sleep_enable_timer_wakeup(secToSleep * uS_TO_S_FACTOR);
  esp_deep_sleep_start();  
}

uint32_t getSecondsToSleep(uint32_t curTimeSec) {
  for (int i=0; timesForImage[i].length() > 0; i++) {
    Serial.printf("Checking %s: \n", timesForImage[i].c_str());
    uint32_t t = timeToSec(timesForImage[i].c_str());
    if (abs(curTimeSec - t) < 90) return 0;
    if (curTimeSec < t) return (t - curTimeSec);
  }
  return (timeToSec("24:00") - curTimeSec + timeToSec(timesForImage[0].c_str()));
}

void loop() {
//  saveCapturedImage();
//  delay(60000);
}

void saveCapturedImage() {
  Serial.println("Connect to " + String(myDomain));
  WiFiClientSecure client;
  
  if (client.connect(myDomain, 443)) {
    Serial.println(F("Connection successful"));
    
    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();  
    if(!fb) {
      Serial.println(F("Camera capture failed"));
      delay(1000);
      ESP.restart();
      return;
    }
  
    char *input = (char *)fb->buf;
    int encLen = base64_enc_len(fb->len);
    int chunkSize = 999;
    char output[base64_enc_len(chunkSize) + 1];
    Serial.printf("Will convert %ld to %ld bytes\n", fb->len, encLen);
    Serial.println(F("Send a captured image to Google Drive."));
    
    client.println("POST " + myScript + " HTTP/1.1");
    client.println("Host: " + String(myDomain));
    client.println("Content-Length: " + String(encLen));
    client.println(F("Content-Type: application/octet-stream"));
    client.println();
    
    unsigned long start = millis();
    for (size_t i=0; i < fb->len;) {
      size_t x = min((size_t)chunkSize, fb->len - i);
      base64_encode(output, input + i, x);
      client.print(output);
      i+=x;
    }
    
    Serial.printf("Image sent in %ld ms \n", millis() - start);
    esp_camera_fb_return(fb);
    
    Serial.println("Waiting for response.");
    start = millis();
    while (!client.available()) {
      Serial.print(".");
      delay(100);
      if ((start + waitingTime) < millis()) {
        Serial.println("\nNo response.");
        //If you have no response, maybe need a greater value of waitingTime
        break;
      }
    }
    Serial.printf("Response Received in %ld ms \n\n", millis() - start);
    while (client.available()) {
      Serial.print(char(client.read()));
    }  
  } else {         
    Serial.println("Connected to " + String(myDomain) + " failed.");
  }
  client.stop();
}

void initWifi() {
  WiFi.mode(WIFI_STA);

  Serial.println("");
  Serial.print(F("Connecting to "));
  Serial.println(ssid);
  WiFi.begin(ssid, password);  

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println(F("STAIP address: "));
  Serial.println(WiFi.localIP());
    
  Serial.println("");

}

void initCamera() {

//  Serial.println("Initializing Camera");
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_UXGA;  // UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
  config.jpeg_quality = 10;
  config.fb_count = 1;
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }  
}

uint32_t getTimeFromGoogleSec() {
  HTTPClient http;
  const char *headers[1] = {"Date"};
  http.begin(F("http://google.com/"));
  http.collectHeaders(headers, 1);
  int rc = http.sendRequest("HEAD");
  if (rc < 0) return -1;
  //String time = http.header("Date");
  const char *d1 = http.header("Date").c_str();
  Serial.printf("Time from Google is: %s \n", d1);
  if (strlen(d1) < 20) return -1;
  uint32_t tg = timeToSec(d1 + strlen(d1) - 12);
  //Serial.printf("from 00:00 is: %ud \n", tg);
  return tg;
}

uint32_t timeToMs(const char *d1) {
  //converts 19:40 to ms from 00:00
  return ( ((d1[0]-'0')*10 + d1[1]-'0')*3600 + 
           ((d1[3]-'0')*10 + d1[4]-'0')*60   +
           ((d1[6]-'0')*10 + d1[7]-'0') ) * 1000;
}

uint32_t timeToSec(const char *d1) {
  //converts 19:40 to ms from 00:00
  return timeToMs(d1)/1000;
}

//void testTimeToSleep() {
//  //timesForImage = {"09:51", "09:56", ""};
//  //uint32_t curTime = timeToMs("09:45");
//  String curTime = "00:00:01";
//  Serial.printf("CurTime %s, minToSleep: %d \n", curTime.c_str(), timeToSec(curTime.c_str()));
//  curTime = "00:01:01";
//  Serial.printf("CurTime %s, minToSleep: %d \n", curTime.c_str(), timeToSec(curTime.c_str()));
////  curTime = "09:51";
////  Serial.printf("CurTime %s, minToSleep: %d \n", curTime.c_str(), getSecondsToSleep(timeToMs(curTime.c_str()))/60);
////  curTime = "09:52";
////  Serial.printf("CurTime %s, minToSleep: %d \n", curTime.c_str(), getSecondsToSleep(timeToMs(curTime.c_str()))/60);
////  curTime = "09:59";
////  Serial.printf("CurTime %s, minToSleep: %d \n", curTime.c_str(), getSecondsToSleep(timeToMs(curTime.c_str()))/60);
//}
