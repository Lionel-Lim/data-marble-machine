#include "secrets.h"
#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#include <FirebaseESP32.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#endif
#include <PubSubClient.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>
#include <ArduinoJson.h>
#include <map>

#define MQTT_SERVER "mqtt.cetools.org"
#define MQTT_TOPIC "UCL/OPS/107/EM/gosund/#"
// Firebase database address
#define DATABASE_URL "https://energycelab-default-rtdb.europe-west1.firebasedatabase.app"

// Define the Firebase Data object
FirebaseData fbdo;
// Define the FirebaseAuth data for authentication data
FirebaseAuth auth;
// Define the FirebaseConfig data for config data
FirebaseConfig config;
// Define Json handling
DynamicJsonDocument incoming(512);
// Define WiFi client
WiFiClient espClient;
// Define MQTT client
PubSubClient client(espClient);

// Define a struct to hold the data for each device
struct DeviceData {
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
// int led = LED_BUILTIN;
std::map<String, DeviceData> deviceList;
FirebaseJson liveOverallJson;
FirebaseJson historyOverallJson;
String defaultPath = "/TEST/";
String liveOverallPath = defaultPath;

void setup()
{
    Serial.begin(115200);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Connecting to Wi-Fi");
    // Try to connect WiFi, LED(13) blinking shows the progress
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(1000);
      Serial.println("Connecting to WiFi...");
    }
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
    /* Assign the RTDB URL */
    config.database_url = DATABASE_URL;

    Firebase.reconnectWiFi(true);
    fbdo.setResponseSize(4096);

    String base_path = "/UsersData/";

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

    /* Initialize the library with the Firebase authen and config */
    Firebase.begin(&config, &auth);

    // Or refresh token manually
    // Firebase.refreshToken(&config);

    client.setServer(MQTT_SERVER, 1883);
    client.setCallback(callback);
    client.setBufferSize(512);

    // Set paths for Firebase
    liveOverallPath += auth.token.uid.c_str();
    liveOverallPath += "/UCL/OPS/107/EM/Live/overall";
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }
  // Check for inactive devices every minute and Update overall data
  if (millis() - liveDataMillis > 60000) {
    for (auto it = deviceList.begin(); it != deviceList.end(); ) {
      if (millis() - it->second.lastUpdated > 60000) {  // 60000 milliseconds = 1 minute
        it = deviceList.erase(it); // remove the device from the list
        deviceCount--; // decrement the device count
      } else {
        ++it;
      }
    }
    liveDataMillis = millis();

    Serial.println(updateOverallLive());
  }
  // Update history data every 30 mins
  if (millis() - historyDataMillis > 1800000) {
    historyDataMillis = millis();
    Serial.println(updateHistory());
  }
  client.loop();
}


void callback(char* topic, byte* payload, unsigned int length) {
  String topicString(topic);
  int lastSlash = topicString.lastIndexOf('/');
  String lastSection = topicString.substring(0, lastSlash);  // Get the string up to the last '/'
  lastSlash = lastSection.lastIndexOf('/');  // Find the position of the second last '/'
  String deviceName = lastSection.substring(lastSlash + 1);  // Extract the section after the second last '/'

  // Check number of device
  if (topicString.indexOf("LWT") != -1 && deviceList.find(deviceName) == deviceList.end()) {
    // TODO : Count the number of device regularly 
    deviceCount++;
    DeviceData data;
    data.lastUpdated = millis();
    deviceList[deviceName] = data;

    Serial.print("Number of devices: ");
    Serial.println(deviceCount);

    Serial.print("Contents of topicList:");
    for (auto& device : deviceList) {
      Serial.print(device.first);
      Serial.print(", ");
    }
    Serial.println("");
  }

  // Check SENSOR value
  if (topicString.indexOf("SENSOR") != -1) {
    DeviceData data;
    data.lastUpdated = millis(); // Record received time
    char msg[length+1]; //Convert the bytes data to char
    memcpy (msg, payload, length);
    msg[length] = '\0';
    DeserializationError error = deserializeJson(incoming, msg); // Convert the Char data to Json format
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    data.time = String(incoming["Time"].as<const char*>());
    data.startDate = String(incoming["ENERGY"]["TotalStartTime"].as<const char*>());
    data.total = incoming["ENERGY"]["Total"];
    data.today = incoming["ENERGY"]["Today"];
    data.yesterday = incoming["ENERGY"]["Yesterday"];
    data.power = incoming["ENERGY"]["Power"];

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

    if (Firebase.pushJSON(fbdo, livePath, liveJson)){
      Serial.printf("Push data... %s\n", "ok");
    }else{
      Serial.printf("Error: \n%s", fbdo.errorReason().c_str());
      refreshFirebase();
    }

    // Serial.printf("Push data... %s\n", Firebase.pushJSON(fbdo, livePath, liveJson) ? "ok" : fbdo.errorReason().c_str());
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("DY_Receiver")) {
      Serial.println("connected");
      Serial.print("Subscription is...");
      Serial.println(client.subscribe(MQTT_TOPIC) ? "Success" : "Fail");
      Serial.print("Subscribed Topic is: ");
      Serial.println(MQTT_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

int sumPower() {
  int totalPower = 0;
  for(auto const& pair : deviceList){
    totalPower += pair.second.power;
  }
  return totalPower;
}

float sumTotalUse() {
  float totalUse = 0;
  for(auto const& pair : deviceList){
    totalUse += pair.second.total;
  }
  return totalUse;
}

float sumTodayUse() {
  float totalUse = 0;
  for(auto const& pair : deviceList){
    totalUse += pair.second.today;
  }
  return totalUse;
}

float sumYesterdayUse() {
  float YesterdayUse = 0;
  for(auto const& pair : deviceList){
    YesterdayUse += pair.second.yesterday;
  }
  return YesterdayUse;
}

String updateOverallLive() {
  liveOverallJson.set("devices", deviceCount);
  liveOverallJson.set("power", sumPower());
  
  if (Firebase.pushJSON(fbdo, liveOverallPath, liveOverallJson)) {
    return "Overall Data:Success";
  } else {
    return fbdo.errorReason().c_str();
  }
}

String updateHistory() {
  String result = "History Data:";
  String historyOverallPath;
  for(auto const& pair : deviceList){
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

    if (Firebase.pushJSON(fbdo, historyPath, historyJson)) {
      result += "Success///";
    } else {
      result += fbdo.errorReason().c_str();
      result += "///";
    }
  }
  historyOverallJson.set("total", sumTotalUse());
  historyOverallJson.set("today", sumTodayUse());
  historyOverallJson.set("yesterday", sumYesterdayUse());
  if (Firebase.pushJSON(fbdo, historyOverallPath, historyOverallJson)) {
    result += "Success///";
  } else {
    result += fbdo.errorReason().c_str();
  }
  
  return result;
}

bool refreshFirebase(){
  Firebase.refreshToken(&config);
  Serial.print("Token refreshed.");
  if(Firebase.ready()){
    Serial.println("Firebase Ready.");
    return true;
  }else{
    Serial.print("Firebase Failed.");
    return false;
  }
}