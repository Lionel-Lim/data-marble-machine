#include "secrets.h"
#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ESP32WebServer.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <ESP8266WebServer.h>
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#include <FirebaseESP8266.h>
#endif

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

#include <DNSServer.h>
#include <WiFiManager.h>

// JSON Library
#include <ArduinoJson.h>

// LED Library
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN        14 // On Trinket or Gemma, suggest changing this to 1

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 40 // Popular NeoPixel ring size

// Define the RTDB URL
#define DATABASE_URL "https://energycelab-default-rtdb.europe-west1.firebasedatabase.app"

#define LIVEOVERALL "/Data/42grHfoBn4hpVYyNBhven4G4Sgk2/UCL/OPS/107/EM/Live/overall"

/* This database secret required in this example to get the righs access to database rules */
#define DATABASE_SECRET "DATABASE_SECRET"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

// Define JSON size
DynamicJsonDocument incomingLive(512);

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

bool taskCompleted = false;
int livePower = 0;
int liveFactor = 15;
float todayUse = 0;
float todayFactor = 20;


void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  WiFiManager wifiManager;

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

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);

  Firebase.reconnectWiFi(true);

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setBrightness(10);
}

void loop()
{
  pixels.clear(); // Set all pixel colors to 'off'
  pixels.setPixelColor(18, pixels.Color(255, 0, 0));

  // Firebase.ready() should be called repeatedly to handle authentication tasks.
  if (Firebase.ready() && !taskCompleted)
  {

    taskCompleted = true;

    // Firebase.setQueryIndex(fbdo, "/test/push" /* parent path of child's node that is being queried */, "Data2" /* the child node key that is being queried */, DATABASE_SECRET);

    QueryFilter query;

    // Use "$key" as the orderBy parameter if the key of child nodes was used for the query
    query.orderBy("$key");

    query.limitToLast(1);

    // Path to your data
    String livePath = "/Data/42grHfoBn4hpVYyNBhven4G4Sgk2/UCL/OPS/107/EM/Live/overall";
    String historyPath = "/Data/42grHfoBn4hpVYyNBhven4G4Sgk2/UCL/OPS/107/EM/History/Overall";

    // Get live Data and set LED
    if (Firebase.getJSON(fbdo, livePath.c_str(), query)) {
      String msg = fbdo.jsonString();
      const char* charMsg = msg.c_str();
      Serial.println(msg);
      DeserializationError error = deserializeJson(incomingLive, charMsg);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
      }else{
        for (JsonPair keyValue : incomingLive.as<JsonObject>()) {
          JsonObject value = keyValue.value();
          livePower = value["power"];
          Serial.println(livePower);
          break;  // Exit the loop after the first key-value pair.
        }
        for(int i = 19; i < 19 + (livePower / liveFactor) ;i++){
          pixels.setPixelColor(i, pixels.Color(232, 229, 88));
        }
      }
    }
    else {
      // Failed to get JSON data at defined database path, print out the error reason
      Serial.println(fbdo.errorReason());
    }

    // Get history data and set LED
    if (Firebase.getJSON(fbdo, historyPath.c_str(), query)) {
      String msg = fbdo.jsonString();
      const char* charMsg = msg.c_str();
      Serial.println(msg);
      DeserializationError error = deserializeJson(incomingLive, charMsg);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
      }else{
        for (JsonPair keyValue : incomingLive.as<JsonObject>()) {
          JsonObject value = keyValue.value();
          todayUse = value["yesterday"];
          Serial.println(int(todayUse * todayFactor));
          break;  // Exit the loop after the first key-value pair.
        }
        for(int i = 17; i > (17 - int(todayUse * todayFactor)) ;i--){
          pixels.setPixelColor(i, pixels.Color(232, 229, 88));
          if(i == 0){
            break;
          }
        }
      }
    }
    else {
      // Failed to get JSON data at defined database path, print out the error reason
      Serial.println(fbdo.errorReason());
    }

    pixels.show();
    Serial.println("Test Passed");

    // Clear all query parameters
    query.clear();
  }
}