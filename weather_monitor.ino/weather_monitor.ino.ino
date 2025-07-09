
#include <TFT_eSPI.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "time.h"

#include "secrets.h"
#include "globals.h"

// Sprites
#include "rainy.c"
#include "rainy_small.c"
#include "sunny.c"
#include "sunny_small.c"
#include "cloudy.c"
#include "cloudy_small.c"

TFT_eSPI tft = TFT_eSPI();

const char* ntpServer = "pool.ntp.org";

uint32_t neonBlue = tft.color565(0,255,255);

struct WeatherData {
  int temperature = 0;
  float percipitationProbability = 0;
  int weatherCode = 0;

  String asString() {
    return "Temp [" + String(temperature) + "] "
           "Rain [" + String(percipitationProbability) + "] "
           "Code [" + String(weatherCode) + "]";
  }
};

struct Time {
  int hour = 0;
  int minute = 0;
  String weekDay = "";
  bool toUpdate = false;
};

Time current_time;

int hour_offset;
WeatherData current_data[5];

unsigned long lastUpdate = 0;
unsigned long lastUpdateTime = 0;

float lat = 0;
float lon = 0;

String getWeekdayAsString(const int day) {
  switch(day) {
    case 1:
      return "MONDAY";
    case 2:
      return "TUESDAY";
    case 3:
      return "WEDNESDAY";
    case 4:
      return "THURSDAY";
    case 5:
      return "FRIDAY";
    case 6:
      return "SATURDAY";
    case 7:
      return "SUNDAY";
  }
}

bool getLocation() 
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

bool getHourOffset()
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

  hour_offset = dstOffset - gmtOffset;

  return true;
}

bool getCurrentHour()
{
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return false;
  }


  if(current_time.minute != timeinfo.tm_min) {
    current_time.hour = timeinfo.tm_hour + hour_offset;
    current_time.minute = timeinfo.tm_min;
    current_time.weekDay = getWeekdayAsString(timeinfo.tm_wday);
    current_time.toUpdate = true;
  }

  return true;
}

void printTime() {

  tft.fillRect(285,85, 150, 80, TFT_BLACK);

  tft.setTextSize(3);
  tft.setCursor(290,85);

  String weekDay = "";

  if(!(current_time.weekDay == "WEDNESDAY" || current_time.weekDay == "THURSDAY" || current_time.weekDay == "SATURDAY"))
    weekDay += " ";

  weekDay += current_time.weekDay;

  tft.println(weekDay);
  tft.setTextSize(5);
  tft.setCursor(285,120);

  String hour = current_time.hour < 10 ? "0" + String(current_time.hour) : String(current_time.hour);
  String minute = current_time.minute < 10 ? "0" + String(current_time.minute) : String(current_time.minute);

  tft.println(hour + ":" + minute);
  current_time.toUpdate = false;
}

void printCurrentWeather() 
{
  tft.fillRect(30,70, 200, 128, TFT_BLACK);
  tft.fillRect(30,250, 440, 50, TFT_BLACK);

  if(current_data[0].weatherCode > 60)
    tft.pushImage(30,70, 128, 128, image_data_rainy);
  else if(current_data[0].weatherCode > 2)
    tft.pushImage(30,70, 128, 128, image_data_cloudy);
  else
    tft.pushImage(30,70, 128, 128, image_data_sunny);

  tft.setTextSize(3);

  tft.setCursor(175,90);

  String temperature = "";
  if(current_data[0].temperature < 10)
    temperature += " ";
  temperature += String(current_data[0].temperature) + "C";

  tft.println(temperature);

  tft.setCursor(175,120);

  String percipitation = "";
  if(current_data[0].percipitationProbability < 10)
    percipitation += " ";
  percipitation += String((int)current_data[0].percipitationProbability) + "%";
  
  tft.println(percipitation);

  tft.setTextSize(2);

  for(int i = 0; i < 4; i++)
  {
    int x = 31 + i * (82 + 30);
    tft.setCursor(x,250);

    String hour = "";
    if(current_time.hour + i + 1 < 10)
      hour += " ";
    hour += String(current_time.hour + i + 1) + "H";

    tft.print(hour);

    if(current_data[i+1].weatherCode > 60)
      tft.pushImage(x+44,240, 32, 32, image_data_rainy_small);
    else if(current_data[i+1].weatherCode > 2)
      tft.pushImage(x+44,240, 32, 32, image_data_cloudy_small);
    else
      tft.pushImage(x+44,240, 32, 32, image_data_sunny_small);

    String temperature_s = "";
    if(current_data[i+1].temperature < 10)
      temperature_s += " ";
    temperature_s += String(current_data[i+1].temperature) + "C";

    String percipitation_s = "";
    percipitation_s += String((int)current_data[i+1].percipitationProbability) + "%";

    tft.setCursor(x,280);
    tft.print(temperature_s + " " + percipitation_s);
  }
}

bool getWeatherData()
{
  DynamicJsonDocument doc(1024);

  String url = URL_GET_WEATHER_DATA;
  url.replace("$lat$", String(lat,6));
  url.replace("$lon$", String(lon,6));
  
  if(!sendHttpRequest(url,doc)) 
  {
    return false;
  } 

  for(int i = 0; i < 5; i++) {
    int data_index = current_time.hour + i;
    current_data[i].temperature = doc["hourly"]["temperature_2m"][data_index];
    current_data[i].percipitationProbability = doc["hourly"]["precipitation_probability"][data_index];
    current_data[i].weatherCode = doc["hourly"]["weather_code"][data_index];
  }

  return true;
}

void setup() 
{
  Serial.begin(115200);
  delay(1000);

  tft.init();
  tft.setRotation(1);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(neonBlue);
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

  configTime(0, 0, ntpServer);

  if(!getLocation())
  {
    tft.println("Error on get location request");
  }
  else
  {
    String location = "Lat: " + String(lat) + ", Lon: " + String(lon);
    tft.println(location);

    if(!getHourOffset())
      tft.println("Error on get hour offset request");

    if(!getCurrentHour())
      tft.println("Error on get current hour request");

    if(!getWeatherData())
      tft.println("Error on get weather data request");
  }

  lastUpdateTime = millis();

  drawFrame();
  printCurrentWeather();
}

void loop() 
{
  unsigned long now = millis();

  if (now - lastUpdate >= UPDATE_DELAY) {
    lastUpdate = now;
    
    if(!getWeatherData())
      tft.println("Error on get weather data request");

    printCurrentWeather();
  }
  else if(now - lastUpdateTime >= UPDATE_TIME_DELAY) {

    if(!getCurrentHour())
      Serial.println("Error on get current hour request");

    if(current_time.toUpdate) printTime();

    lastUpdateTime = now;
  }
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

void drawFrame()
{
  tft.fillScreen(TFT_BLACK);

  // Frame
  tft.drawLine(0,10,10,0,neonBlue);
  tft.drawLine(10,0,469,0,neonBlue);
  tft.drawLine(469,0,479,10,neonBlue);
  tft.drawLine(0,10,0,309,neonBlue);
  tft.drawLine(0,309,10,319,neonBlue);
  tft.drawLine(10,319,469,319,neonBlue);
  tft.drawLine(469,319,479,309,neonBlue);
  tft.drawLine(479,309,479,10,neonBlue);

  // Title
  tft.setCursor(110,14);
  tft.setTextSize(3);
  tft.print("WEATHER MONITOR");
  tft.drawLine(20,40,459,40,neonBlue);
}
