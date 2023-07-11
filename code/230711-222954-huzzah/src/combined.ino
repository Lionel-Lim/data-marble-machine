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

// MQTT Library
#include <PubSubClient.h>

#include <map>

// MQTT Server address
// TODO: Change this address dynamically in App or Web
#define MQTT_SERVER "mqtt.cetools.org"
#define MQTT_TOPIC "UCL/OPS/107/EM/gosund/#"

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN 14 // On Trinket or Gemma, suggest changing this to 1

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 40 // Popular NeoPixel ring size

// Define the RTDB URL
#define DATABASE_URL "https://energycelab-default-rtdb.europe-west1.firebasedatabase.app"

#define LIVEOVERALL "/Data/42grHfoBn4hpVYyNBhven4G4Sgk2/UCL/OPS/107/EM/Live/overall"

/* This database secret required in this example to get the righs access to database rules */
#define DATABASE_SECRET "DATABASE_SECRET"

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Define JSON size
DynamicJsonDocument incomingLive(256);
DynamicJsonDocument MQTTincoming(512);

// Define NeoPixel object
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Define WiFi client
WiFiClient espClient;
// Define MQTT client
PubSubClient mqttClient(espClient);

// Define a data structure for MQTT data processing
struct DeviceData
{
  unsigned long lastUpdated;
  int power;
  float today;
  float yesterday;
  float total;
  String time;
  String startDate;
};

unsigned long liveDataMillis = 0;
unsigned long historyDataMillis = 0;
unsigned long testMillis = 0;
int deviceCount = 0;
int led = LED_BUILTIN;
std::map<String, DeviceData> deviceList;
FirebaseJson liveOverallJson;
FirebaseJson historyOverallJson;
String defaultPath = "/TEST/";
String liveOverallPath = defaultPath;
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
  pinMode(led, OUTPUT);
  for (int i = 0; i < 5; i++)
  {
    digitalWrite(led, HIGH);
    delay(1000);
    digitalWrite(led, LOW);
    delay(1000);
  }

  WiFiManager wifiManager;
  // Uncomment to reset saved settings
  // wifiManager.resetSettings();

  wifiManager.setAPCallback([](WiFiManager *myWiFiManager)
                            { Serial.println("Connect to the AP named \"ESPap\" and open any web page to setup."); });

  // Fetches ssid and password, and tries to connect,
  // if it does not connect, it starts an access point with the specified name
  // and goes into a blocking loop awaiting configuration.
  if (!wifiManager.autoConnect("MarbleMachine", "thereisnospoon"))
  {
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
  // auth.user.email = USER_EMAIL;
  // auth.user.password = USER_PASSWORD;
  auth.user.email = TEST_EMAIL;
  auth.user.password = TEST_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);

  Firebase.reconnectWiFi(true);

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setBrightness(10);

  // Set MQTT Server
  mqttClient.setServer(MQTT_SERVER, 1883);
  mqttClient.setCallback(callback);
  mqttClient.setBufferSize(512);

  // Set paths for Firebase
  liveOverallPath += auth.token.uid.c_str();
  liveOverallPath += "/UCL/OPS/107/EM/Live/overall";
}

void loop()
{
  // MQTT Client
  if (!mqttClient.connected())
  {
    Serial.println("MQTT Client not connected");
    reconnect();
  }

  // Check for inactive devices every minute and Update overall data
  if (millis() - liveDataMillis > 60000)
  {
    for (auto it = deviceList.begin(); it != deviceList.end();)
    {
      if (millis() - it->second.lastUpdated > 60000)
      {                            // 60000 milliseconds = 1 minute
        it = deviceList.erase(it); // remove the device from the list
        deviceCount--;             // decrement the device count
      }
      else
      {
        ++it;
      }
    }
    liveDataMillis = millis();

    Serial.println(updateOverallLive());
  }

  // Update history data every 30 mins
  if (millis() - historyDataMillis > 1800000)
  {
    historyDataMillis = millis();
    Serial.println(updateHistory());
  }

  pixels.clear(); // Set all pixel colors to 'off'
  pixels.setPixelColor(18, pixels.Color(255, 0, 0));

  // Firebase.ready() should be called repeatedly to handle authentication tasks.
  if (Firebase.ready() && !taskCompleted)
  {
    taskCompleted = true;

    QueryFilter query;

    // Use "$key" as the orderBy parameter if the key of child nodes was used for the query
    query.orderBy("$key");
    query.limitToLast(1);

    // Path to your data
    String livePath = "/Data/42grHfoBn4hpVYyNBhven4G4Sgk2/UCL/OPS/107/EM/Live/overall";
    String historyPath = "/Data/42grHfoBn4hpVYyNBhven4G4Sgk2/UCL/OPS/107/EM/History/Overall";

    // Get live Data and set LED
    if (Firebase.getJSON(fbdo, livePath.c_str(), query))
    {
      String msg = fbdo.jsonString();
      const char *charMsg = msg.c_str();
      Serial.println(msg);
      DeserializationError error = deserializeJson(incomingLive, charMsg);
      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
      }
      else
      {
        for (JsonPair keyValue : incomingLive.as<JsonObject>())
        {
          JsonObject value = keyValue.value();
          livePower = value["power"];
          Serial.println(livePower);
          break; // Exit the loop after the first key-value pair.
        }
        // TODO: Calculate the number of LED to light up based on the average over time
        for (int i = 19; i < 19 + (livePower / liveFactor); i++)
        {
          pixels.setPixelColor(i, pixels.Color(232, 229, 88));
        }
      }
    }
    else
    {
      // Failed to get JSON data at defined database path, print out the error reason
      Serial.println(fbdo.errorReason());
    }

    // Get history data and set LED
    if (Firebase.getJSON(fbdo, historyPath.c_str(), query))
    {
      String msg = fbdo.jsonString();
      const char *charMsg = msg.c_str();
      Serial.println(msg);
      DeserializationError error = deserializeJson(incomingLive, charMsg);
      if (error)
      {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
      }
      else
      {
        for (JsonPair keyValue : incomingLive.as<JsonObject>())
        {
          JsonObject value = keyValue.value();
          todayUse = value["yesterday"];
          Serial.println(int(todayUse * todayFactor));
          break; // Exit the loop after the first key-value pair.
        }
        for (int i = 17; i > (17 - int(todayUse * todayFactor)); i--)
        {
          pixels.setPixelColor(i, pixels.Color(232, 229, 88));
          if (i == 0)
          {
            break;
          }
        }
      }
    }
    else
    {
      // Failed to get JSON data at defined database path, print out the error reason
      Serial.println(fbdo.errorReason());
    }

    pixels.show();
    Serial.println("Test Passed");

    // Clear all query parameters
    query.clear();
  }

  // Regularly check for MQTT connection
  mqttClient.loop();
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String topicString(topic);
  int lastSlash = topicString.lastIndexOf('/');
  String lastSection = topicString.substring(0, lastSlash); // Get the string up to the last '/'
  lastSlash = lastSection.lastIndexOf('/');                 // Find the position of the second last '/'
  String deviceName = lastSection.substring(lastSlash + 1); // Extract the section after the second last '/'

  // Check number of device
  if (topicString.indexOf("LWT") != -1 && deviceList.find(deviceName) == deviceList.end())
  {
    // TODO : Count the number of device regularly
    deviceCount++;
    DeviceData data;
    data.lastUpdated = millis();
    deviceList[deviceName] = data;

    Serial.print("Number of devices: ");
    Serial.println(deviceCount);

    Serial.print("Contents of topicList:");
    for (auto &device : deviceList)
    {
      Serial.print(device.first);
      Serial.print(", ");
    }
    Serial.println("");
  }

  // Check SENSOR value
  if (topicString.indexOf("SENSOR") != -1)
  {
    DeviceData data;
    data.lastUpdated = millis(); // Record received time
    char msg[length + 1];        // Convert the bytes data to char
    memcpy(msg, payload, length);
    msg[length] = '\0';
    DeserializationError error = deserializeJson(MQTTincoming, msg); // Convert the Char data to Json format
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    data.time = String(MQTTincoming["Time"].as<const char *>());
    data.startDate = String(MQTTincoming["ENERGY"]["TotalStartTime"].as<const char *>());
    data.total = MQTTincoming["ENERGY"]["Total"];
    data.today = MQTTincoming["ENERGY"]["Today"];
    data.yesterday = MQTTincoming["ENERGY"]["Yesterday"];
    data.power = MQTTincoming["ENERGY"]["Power"];

    deviceList[deviceName] = data;

    String livePath = defaultPath;
    livePath += auth.token.uid.c_str();
    livePath += "/UCL/OPS/107/EM/Live/";
    livePath += deviceName;

    FirebaseJson liveJson;
    liveJson.set("time", data.time);
    liveJson.set("power", data.power);

    liveOverallJson.set("time", data.time);
    historyOverallJson.set("time", data.time);

    if (Firebase.pushJSON(fbdo, livePath, liveJson))
    {
      Serial.printf("Push data... %s\n", "ok");
    }
    else
    {
      Serial.printf("Error: \n%s", fbdo.errorReason().c_str());
      refreshFirebase();
    }

    // Serial.printf("Push data... %s\n", Firebase.pushJSON(fbdo, livePath, liveJson) ? "ok" : fbdo.errorReason().c_str());
  }
}

void reconnect()
{
  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect("DY_Device"))
    {
      Serial.println("connected");
      Serial.print("Subscription is...");
      Serial.println(mqttClient.subscribe(MQTT_TOPIC) ? "Success" : "Fail");
      Serial.print("Subscribed Topic is: ");
      Serial.println(MQTT_TOPIC);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

int sumPower()
{
  int totalPower = 0;
  for (auto const &pair : deviceList)
  {
    totalPower += pair.second.power;
  }
  return totalPower;
}

float sumTotalUse()
{
  float totalUse = 0;
  for (auto const &pair : deviceList)
  {
    totalUse += pair.second.total;
  }
  return totalUse;
}

float sumTodayUse()
{
  float totalUse = 0;
  for (auto const &pair : deviceList)
  {
    totalUse += pair.second.today;
  }
  return totalUse;
}

float sumYesterdayUse()
{
  float YesterdayUse = 0;
  for (auto const &pair : deviceList)
  {
    YesterdayUse += pair.second.yesterday;
  }
  return YesterdayUse;
}

String updateOverallLive()
{
  liveOverallJson.set("devices", deviceCount);
  liveOverallJson.set("power", sumPower());

  if (Firebase.pushJSON(fbdo, liveOverallPath, liveOverallJson))
  {
    return "Overall Data:Success";
  }
  else
  {
    return fbdo.errorReason().c_str();
  }
}

String updateHistory()
{
  String result = "History Data:";
  String historyOverallPath;
  for (auto const &pair : deviceList)
  {
    String historyPath = defaultPath;
    historyPath += auth.token.uid.c_str();
    historyPath += "/UCL/OPS/107/EM/History/";
    historyOverallPath = historyPath;
    historyOverallPath += "Overall";
    historyPath += pair.first;

    FirebaseJson historyJson;
    historyJson.set("time", pair.second.time);
    historyJson.set("total", pair.second.total);
    historyJson.set("today", pair.second.today);
    historyJson.set("yesterday", pair.second.yesterday);
    historyJson.set("startDate", pair.second.startDate);

    if (Firebase.pushJSON(fbdo, historyPath, historyJson))
    {
      result += "Success///";
    }
    else
    {
      result += fbdo.errorReason().c_str();
      result += "///";
    }
  }
  historyOverallJson.set("total", sumTotalUse());
  historyOverallJson.set("today", sumTodayUse());
  historyOverallJson.set("yesterday", sumYesterdayUse());
  if (Firebase.pushJSON(fbdo, historyOverallPath, historyOverallJson))
  {
    result += "Success///";
  }
  else
  {
    result += fbdo.errorReason().c_str();
  }

  return result;
}

bool refreshFirebase()
{
  Firebase.refreshToken(&config);
  Serial.print("Token refreshed.");
  if (Firebase.ready())
  {
    Serial.println("Firebase Ready.");
    return true;
  }
  else
  {
    Serial.print("Firebase Failed.");
    return false;
  }
}