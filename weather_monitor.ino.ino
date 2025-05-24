#include <TFT_eSPI.h>
#include <WiFi.h>
#include "secrets.h"

TFT_eSPI tft = TFT_eSPI();

const int reconnect_delay = 10;

void setup() {
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

    if(curr_connection_attempt >= reconnect_delay)
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

  tft.unloadFont();
}

void loop() {
  // put your main code here, to run repeatedly:

}
