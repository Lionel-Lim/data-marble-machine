#include <ESP8266WiFi.h>  // For ESP8266
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

void setup() {
  Serial.begin(115200);
  
  WiFiManager wifiManager;

  // Uncomment and run it once, after uploading the sketch,
  // then disable to reset settings for testing.
  // wifiManager.resetSettings();

  wifiManager.setAPCallback([](WiFiManager* myWiFiManager) {
    Serial.println("Connect to the AP named \"ESPap\" and open any web page to setup.");
  });

  // Fetches ssid and password, and tries to connect, 
  // if it does not connect, it starts an access point with the specified name
  // and goes into a blocking loop awaiting configuration.
  if (!wifiManager.autoConnect("ESPap", "thereisnospoon")) {
    Serial.println("Failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }

  // If you get here you have connected to the WiFi
  Serial.println("Connected.");
}

void loop() {
  // Your application code here
}
