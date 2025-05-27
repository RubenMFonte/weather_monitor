
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "secrets.h"

TFT_eSPI tft = TFT_eSPI();
HTTPClient http;

const int RECONNECT_DELAY = 10;
const int HTTP_SUCCESS = 200;

const String URL_GET_LOCATION = "http://ip-api.com/json/";

bool getLocation(float& lat, float& lon) 
{
  http.begin(URL_GET_LOCATION);

  int httpCode = http.GET();

  if (httpCode != HTTP_SUCCESS) {
    http.end();
    return false;
  }

  String payload = http.getString();

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, payload);

  if(error) return false;

  lat = doc["lat"];
  lon = doc["lon"];

  http.end();
  return true;
}

void setup() 
{
  Serial.begin(115200);
  delay(1000);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);

  WiFi.disconnect(true);

  tft.print("Connecting to Wifi...");
  WiFi.begin(SSID,PASSWORD);

  int curr_connection_attempt = 0;

  while(WiFi.status() != WL_CONNECTED)
  {
    curr_connection_attempt++;
    delay(1000);
    tft.print(".");

    if(curr_connection_attempt >= RECONNECT_DELAY)
    {
      tft.println("");
      curr_connection_attempt = 0;
      WiFi.begin(SSID,PASSWORD);
      tft.print("Connection failed, retrying...");
    }
  }
  
  tft.println("");
  tft.println("Wifi connected!");
  tft.print("IP Address: ");
  tft.println(WiFi.localIP());

  float lat = 0;
  float lon = 0;

  if(!getLocation(lat,lon))
  {
    tft.println("Error getting location");
  }
  else
  {
    String location = "Lat: " + String(lat) + ", Lon: " + String(lon);
    tft.println(location);
  }

  tft.unloadFont();
}

void loop() 
{
  // put your main code here, to run repeatedly:

}
