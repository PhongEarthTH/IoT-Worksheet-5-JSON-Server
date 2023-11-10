#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

float tem;
float hum;
unsigned long count = 1;
const char* ssid = "Galaxy A71 96DE";
const char* password = "25452002";
const char* server = "http://192.168.124.66:3000/data";
WiFiClient client;
HTTPClient http;
DHT dht11(D4, DHT11);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 0);

void ReadDHT11() {
  tem = dht11.readTemperature();
  hum = dht11.readHumidity();
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  dht11.begin();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  timeClient.begin();
}

void loop() {
  static unsigned long lastTime = 0;
  unsigned long timerDelay = 15000;

  if ((millis() - lastTime) > timerDelay) {
    ReadDHT11();
    
    timeClient.update();
    
    String currentDateTime = getFormattedDateTime();

    if (isnan(hum) || isnan(tem)) {
      Serial.println("Failed");
    } else {
      Serial.printf("Humidity: %.2f%%\n", hum);
      Serial.printf("temerature: %.2f°C\n", tem);

      StaticJsonDocument<200> jsonDocument;
      jsonDocument["humidity"] = hum;
      jsonDocument["temerature"] = tem;
      jsonDocument["dateTime"] = currentDateTime;

      String jsonData;
      serializeJson(jsonDocument, jsonData);

      http.begin(client, server);
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(jsonData);

      if (httpResponseCode > 0) {
        Serial.println("HTTP Response code: " + String(httpResponseCode));
        String payload = http.getString();
        Serial.println("Returned payload:");
        Serial.println(payload);
        count += 1;
      } else {
        Serial.println("Error on sending POST: " + String(httpResponseCode));
      }
      http.end();
    }
    lastTime = millis();
  }
}

String getFormattedDateTime() {
  time_t now = timeClient.getEpochTime() + 6 * 3600;
  struct tm *tmstruct = localtime(&now);

  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
           tmstruct->tm_year + 1900, tmstruct->tm_mon + 1, tmstruct->tm_mday,
           tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);

  return String(buffer);
}
