#include <WiFi.h>
#include <TFT_eSPI.h> // it seems TFT_eSPI does not work on versions higher than 2.0.14 
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <FastLED.h>
#include <TimeLib.h> // timekeeping lib
#include "OneButton.h"
#include "weather_condition_imgs.h"
#include "pin_config.h"
#include "secrets.h"
#include "weatherParams.h"

CRGB leds;

// OneButton setup on pin A1. 
OneButton button(BTN_PIN, true);

// Secrets
String openWeatherMapKey = SECRET_OWM_KEY;
String ssid = SECRET_SSID;
String ssidPass = SECRET_PASS;

// Params
const String city = CITY;
const String countryCode = COUNTRY_CODE;
const String lang = LANG;

unsigned long previousMillis = 0;
const unsigned long interval = 32000; // 5 minutes
bool started = false;
bool showWeatherConditionDetails = true;

String jsonBuffer;
int weatherConditionId;
String serverPath = "http://api.openweathermap.org/data/2.5/forecast?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapKey + "&units=metric" + "&lang=" + lang;

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  connectToWifi();
  FastLED.addLeds<APA102, LED_DI_PIN, LED_CI_PIN, BGR>(&leds, 1);

  button.attachClick([] {
    started = false;
    showWeatherConditionDetails = !showWeatherConditionDetails;
  });
  
  initTFT();
}

void loop() {

  // keep watching the push button:
  button.tick();
  
  if ((millis() - previousMillis >= interval) || !started) {
    
    started = true;
    previousMillis = millis();
  
    if(WiFi.status()== WL_CONNECTED){
  
      jsonBuffer = httpGETRequest(serverPath.c_str());
      JSONVar myObject = JSON.parse(jsonBuffer);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        //tft.println("Parsing input failed!");
        return;
      }
  
      // datetime
      //tft.println(day(myObject["list"][0]["dt"]) + "/" + month(myObject["list"][0]["dt"]));
      
      // Weather condition id for the next 3 hours
      weatherConditionId = myObject["list"][0]["weather"][0]["id"];
  
      // TFT images 
      if (weatherConditionId >= 200 && weatherConditionId <= 232){ // Thunderstorm 
         updateImageAndLed(ow11d, 85, 255);
      } else if (weatherConditionId >= 300 && weatherConditionId <= 321){ // Drizzle 
         updateImageAndLed(ow09d, 89, 255);
      } else if (weatherConditionId >= 500 && weatherConditionId <= 504){ // rain 
         updateImageAndLed(ow10d, 121, 255);
      } else if (weatherConditionId == 511){ // freezing rain 
         updateImageAndLed(ow13d, 41, 255); 
      } else if (weatherConditionId >= 520 && weatherConditionId <= 531){ // shower rain 
         updateImageAndLed(ow09d, 176, 255); 
      } else if (weatherConditionId >= 600 && weatherConditionId <= 622){ // snow
         updateImageAndLed(ow13d, 199, 255); 
      } else if (weatherConditionId >= 701 && weatherConditionId <= 781){ // mist
         updateImageAndLed(ow50d, 189, 255); 
      } else if (weatherConditionId == 800){ // clear sky
         updateImageAndLed(ow01d, 84, 255);
      } else if (weatherConditionId == 801){// few clouds 
         updateImageAndLed(ow02d, 204, 255);
      } else if (weatherConditionId == 802){ // scattered clouds 
         updateImageAndLed(ow03d, 39, 255);
      } else if (weatherConditionId >= 803 && weatherConditionId <= 804){ // broken clouds 
         updateImageAndLed(ow04d, 214, 255);
      } else {
        updateImageAndLed(weatherNotRecognized, 0, 255);
        ledError();
      }

      tft.setCursor(117,16);
      tft.setTextColor(TFT_BLACK);

      if (showWeatherConditionDetails) {
        // Temperature and Humidity
        tft.print(int(myObject["list"][0]["main"]["temp"]));
        tft.print("`C");
        tft.setCursor(117,32);
        tft.print(myObject["list"][0]["main"]["humidity"]);
        tft.print("%");
        
        // Weather condition
        tft.setTextColor(TFT_WHITE);
        tft.fillRect(0,62,160,30, TFT_BLACK);
        
        tft.setCursor(2,75);
        String description = myObject["list"][0]["weather"][0]["description"];
        description.replace("\"", "");
        tft.println(description);
      }
    }
  }
}

// WiFi

void connectToWifi(){
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, ssidPass);

  tft.print("Connecting to WiFi Network ..");

  while(WiFi.status() != WL_CONNECTED){
    delay(100);
  }

  tft.println("\nConnected to the WiFi network: ");
  tft.println(WiFi.localIP());
}

// LED

void ledError() {
  leds = CHSV(0, 255, 255);
  FastLED.show();
}

void updateImageAndLed(const uint16_t* weatherConditionIconId, int ledHue, int saturation){
  tft.pushImage(0, 0, 160, 80, weatherConditionIconId);
  // TODO manage if it's nighttime
  leds = CHSV(ledHue, saturation, 255);
  FastLED.show();
  button.tick();
}

// TFT

void initTFT(){
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);
  tft.setFreeFont(&FreeSans9pt7b);
  tft.setTextColor(TFT_GREEN);
  //tft.setTextSize(1);
}

// Open Weather Map

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverName);
  
  int httpResponseCode = http.GET();
  button.tick();
  String payload = "{}"; 

  // keep button alive
  button.tick();

  if (httpResponseCode>0) {
    payload = http.getString();
  }
  else {
    tft.fillScreen(TFT_BLACK);
    tft.println("Error code: ");
    tft.println(httpResponseCode);
  }
  
  http.end();
  button.tick();
  return payload;
}
