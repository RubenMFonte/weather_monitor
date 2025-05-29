
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "secrets.h"

TFT_eSPI tft = TFT_eSPI();

const int RECONNECT_DELAY = 10;
const int HTTP_SUCCESS = 200;

const String URL_GET_LOCATION = "http://ip-api.com/json/";
const String URL_GET_TIME = "https://timeapi.io/api/time/current/ip?ipAddress=$ip$";
const String URL_GET_WEATHER_DATA = "https://api.open-meteo.com/v1/forecast?latitude=$lat$&longitude=$lon$&hourly=temperature_2m,precipitation_probability,weather_code&forecast_days=1";
const String URL_GET_HOUR_OFFSET = "http://api.geonames.org/timezoneJSON?lat=$lat$&lng=$lon$&username=$username$";

bool getLocation(float& lat, float& lon) 
{
  DynamicJsonDocument doc(1024);
  
  if(!sendHttpRequest(URL_GET_LOCATION,doc)) 
  {
    return false;
  } 

  lat = doc["lat"];
  lon = doc["lon"];

  return true;
}

bool getHourOffset(const float lat, const float lon, int& offset)
{
  DynamicJsonDocument doc(1024);

  String url = URL_GET_HOUR_OFFSET;
  url.replace("$lat$", String(lat,6));
  url.replace("$lon$", String(lon,6));
  url.replace("$username$", GEONAME_USERNAME);
  
  if(!sendHttpRequest(url,doc)) 
  {
    return false;
  } 
  
  int gmtOffset = doc["gmtOffset"];
  int dstOffset = doc["dstOffset"];

  offset = dstOffset - gmtOffset;
  
  tft.println("Hour offset: " + String(offset));

  return true;
}

bool getCurrentHour(const String ip, int& hour)
{
  DynamicJsonDocument doc(1024);

  String url = URL_GET_TIME;
  url.replace("$ip$", ip);
  
  if(!sendHttpRequest(url,doc)) 
  {
    return false;
  } 
  
  hour = doc["hour"];

  return true;
}

bool getWeatherData(const float lat, const float lon)
{
  DynamicJsonDocument doc(1024);

  String url = URL_GET_WEATHER_DATA;
  url.replace("$lat$", String(lat,6));
  url.replace("$lon$", String(lon,6));
  
  if(!sendHttpRequest(url,doc)) 
  {
    return false;
  } 

  float temperature = doc["hourly"]["temperature_2m"][0];
  int precipitation_probability = doc["hourly"]["precipitation_probability"][0];
  int weather_code = doc["hourly"]["weather_code"][0];
  
  tft.println("Temp: " + String(temperature,2));
  tft.println("Precipitation: " + String(precipitation_probability));
  tft.println("Code: " + String(weather_code));

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
    tft.println("Error on get location request");
  }
  else
  {
    String location = "Lat: " + String(lat) + ", Lon: " + String(lon);
    tft.println(location);

    int current_hour = 0;
    int hour_offset = 0;

    if(!getHourOffset(lat,lon,hour_offset))
      tft.println("Error on get hour offset request");

    if(!getCurrentHour(WiFi.localIP().toString(),current_hour))
      tft.println("Error on get current hour request");

    current_hour += hour_offset;

    tft.println("Current hour: " + String(current_hour));

    if(!getWeatherData(lat, lon))
      tft.println("Error on get weather data request");
  }
}

void loop() 
{
  // put your main code here, to run repeatedly:

}

bool sendHttpRequest(const String url, DynamicJsonDocument& doc) 
{
  HTTPClient http;

  http.begin(url);

  int httpCode = http.GET();

  if (httpCode != HTTP_SUCCESS) 
  {
    http.end();
    return false;
  }

  String payload = http.getString();

  DeserializationError error = deserializeJson(doc, payload);

  if(error) return false;

  http.end();
  return true;
}
