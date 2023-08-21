#include <cmath>
#include "secrets.h"
#include <Arduino.h>
#if defined(ESP32)
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#elif defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#include <FirebaseESP8266.h>
#endif

// Provide the token generation process info.
#include <addons/TokenHelper.h>

// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>

#include <DNSServer.h>
#include <WebServer.h>
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

// Adafruit Motor Shield Library
#include <Adafruit_MotorShield.h>

#include <map>

// Time Library
#include "time.h"

// MQTT Server address
// TODO: Change this address dynamically in App or Web
#define MQTT_SERVER "mqtt.cetools.org"
#define MQTT_TOPIC "UCL/OPS/107/EM/gosund/#"

#define LED_PIN 13

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 40  // Popular NeoPixel ring size
#define CENTRE_LED 19 // Centre LED index

#define PROJECT_ID "energycelab"

#define TOUCH_PIN 27

#define BUTTON1_PIN 15
#define BUTTON2_PIN 14

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Define JSON size
DynamicJsonDocument incomingLive(256);
DynamicJsonDocument MQTTincoming(512);
DynamicJsonDocument firestoreJSON(1024);
DynamicJsonDocument preferenceJSON(256);

// Define NeoPixel object
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Define WiFi client
WiFiClient espClient;
// Define MQTT client
PubSubClient mqttClient(espClient);

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
// Select which 'port' M1, M2, M3 or M4. In this case, M1
Adafruit_DCMotor *myMotor = AFMS.getMotor(1);

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

//
// Define global variables
//
unsigned long liveDataMillis = 0;
unsigned long historyDataMillis = 0;
unsigned long testMillis = 0;
unsigned long liveTransitionMillis = 0;
unsigned long dailyLEDMillis = 0;
unsigned long lastTransitionMillis = 0;
unsigned long lastSaveAnimationLEDMillis = 0;
unsigned long lastSaveMillis = 0;
unsigned long Button1StateChangeTime = 0;
unsigned long Button2StateChangeTime = 0;
const unsigned long DebounceTime = 10;
int deviceCount = 0;
int transitionWait = 250;
int saveWait = 3000;
int liveLEDCount = 0;
int lastLEDCount = 0;
int historyLEDCount = 0;
int saveAnimationLastLEDCount = NUMPIXELS;
int liveLEDDefault = 19;
int maximumLivePower = 300;
int maxLiveLED = 19;
int motorSpeed = 0;
int savedMarble = 0;
int targetMarble = 0;
int maxMarble = 10;
int isTouched = 0;
std::map<String, DeviceData> deviceList;
FirebaseJson liveOverallJson;
FirebaseJson historyOverallJson;
String defaultPath = "device/";
String userUID;
bool initialising = true;
bool liveLEDInitialising = true;
bool saveAnimationInit = true;
bool isDebug = false;
bool debugInit = true;
bool Button1Pressed = false;
bool Button2Pressed = false;
bool saveMarbleRequired = false;
bool isSavingFinished = false;
bool isLEDOn = true;
bool stopWheel = false;
bool stopSaving = false;
unsigned int livePower = 0;
unsigned int liveFactor = 15;
float todayFactor = 20;
// TODO: Read this from user preference
double unitEnergyCost = 33.2;
int targetCost = 50;

// Time Library Parameters
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

bool getUserPreference(String path = defaultPath.c_str())
{
  // Print path using printf
  Serial.printf("path: %s\n", path.c_str());
  FirebaseJson preferenceJSON;
  bool isRead = Firebase.Firestore.getDocument(&fbdo, PROJECT_ID, "", path);
  isRead ? Serial.println("Read Success") : Serial.println("Read Fail");
  FirebaseJson json;
  FirebaseJsonData result;
  json.setJsonData(fbdo.payload());
  if (!json.get(result, "fields/preference"))
  {
    Serial.println("User preference not found. Creating new preference.");
    preferenceJSON.set("fields/preference/mapValue/fields/unitCost/doubleValue", unitEnergyCost);
    preferenceJSON.set("fields/preference/mapValue/fields/targetCost/integerValue", targetCost);
    preferenceJSON.set("fields/preference/mapValue/fields/isLEDOn/booleanValue", isLEDOn);
    preferenceJSON.set("fields/preference/mapValue/fields/isForceStop/booleanValue", isDebug);
    preferenceJSON.set("fields/preference/mapValue/fields/stopWheel/booleanValue", stopWheel);
    preferenceJSON.set("fields/preference/mapValue/fields/stopSaving/booleanValue", stopSaving);

    Firebase.Firestore.patchDocument(&fbdo, PROJECT_ID, "", path, preferenceJSON.raw(), "preference");
    return true;
  }
  else
  {
    Serial.println("User preference found.");
    json.setJsonData(fbdo.payload());
    if (json.get(result, "fields/preference/mapValue/fields/unitCost/doubleValue"))
    {
      unitEnergyCost = result.to<double>();
    }
    if (json.get(result, "fields/preference/mapValue/fields/targetCost/integerValue"))
    {
      targetCost = result.to<int>();
    }
    if (json.get(result, "fields/preference/mapValue/fields/isLEDOn/booleanValue"))
    {
      isLEDOn = result.to<bool>();
    }
    if (json.get(result, "fields/preference/mapValue/fields/isForceStop/booleanValue"))
    {
      isDebug = result.to<bool>();
    }
    if (json.get(result, "fields/preference/mapValue/fields/stopWheel/booleanValue"))
    {
      stopWheel = result.to<bool>();
    }
    if (json.get(result, "fields/preference/mapValue/fields/stopSaving/booleanValue"))
    {
      stopSaving = result.to<bool>();
    }
    return true;
  }

  return false;
}

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear(); // Set all pixel colors to 'off'
  pixels.setBrightness(50);
  rainbowFade2White(5, 3, 3);
  pixels.clear(); // Set all pixel colors to 'off'

  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

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
    // Reset device
    ESP.restart();
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

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  // Limit the size of response payload to be collected in FirebaseData
  fbdo.setResponseSize(2048);

  Firebase.begin(&config, &auth);
  Serial.println(Firebase.ready() ? "Firebase is Ready." : "Firebase is Not ready.");

  Firebase.reconnectWiFi(true);

  // Save the user UID
  userUID = auth.token.uid.c_str();
  // Update the default path
  defaultPath += userUID;
  defaultPath += "/";

  // Set MQTT Server
  mqttClient.setServer(MQTT_SERVER, 1883);
  mqttClient.setCallback(callback);
  mqttClient.setBufferSize(512);

  if (!AFMS.begin())
  { // create with the default frequency 1.6KHz
    // if (!AFMS.begin(1000)) {  // OR with a different frequency, say 1KHz
    Serial.println("Could not find Motor Shield. Check wiring.");
    while (1)
      ;
  }
  Serial.println("Motor Shield found.");

  // init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  Serial.println(getCurrentTime());

  // Touch sensor
  pinMode(33, INPUT_PULLUP);

  getUserPreference(defaultPath.c_str());
}

void loop()
{
  // if time is after 7pm and before 8am, turn off the LED using timeinfo
  struct tm tinfo;
  getLocalTime(&tinfo);
  // Serial.printf("Hour: %d\n", tinfo.tm_hour);
  if (tinfo.tm_hour >= 19 && tinfo.tm_hour <= 8)
  // if (tinfo.tm_min >= 7 && tinfo.tm_min < 8)
  {
    isDebug = true;
  }
  else
  {
    isDebug = false;
  }

  checkButton1();
  checkButton2();
  if (isDebug)
  {
    myMotor->setSpeed(0);
    myMotor->run(RELEASE);
    pixels.clear();
  }
  else
  {
    ////////////////////////////////
    // Connect MQTT Client
    ////////////////////////////////
    if (!mqttClient.connected())
    {
      Serial.println("MQTT Client not connected");
      reconnect();
    }

    ////////////////////////////////
    // Check for inactive devices every minute
    // Update overall data
    // Update User preference
    ////////////////////////////////
    if (millis() - liveDataMillis > 60000 && !saveMarbleRequired)
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
      // Update Live Overall data
      Serial.println(updateOverallLive());

      // Check marble saving
      if (stopSaving == false)
      {
        saveMarbleRequired = checkMarbleSaving();
      }

      // Refresh User Preference
      getUserPreference(defaultPath.c_str());

      liveDataMillis = millis();
    }

    ////////////////////////////////
    // Update history data every 30 mins
    ////////////////////////////////
    if (millis() - historyDataMillis > 1800000 && !saveMarbleRequired)
    {
      historyDataMillis = millis();
      Serial.println(updateHistory());
    }

    ///////////////////////////////
    // Get live Data for transition animation
    ///////////////////////////////
    if (Firebase.ready() && !saveMarbleRequired)
    {
      if (millis() - liveTransitionMillis > 60000 || initialising)
      {
        liveTransitionMillis = millis();

        String queryPath = defaultPath;
        queryPath += "sensors/overall/";
        FirebaseJson query;

        query.set("select/fields/[0]/fieldPath", "power");
        query.set("select/fields/[1]/fieldPath", "time");
        query.set("from/collectionId", "live");
        query.set("from/allDescendants", false);
        query.set("orderBy/field/fieldPath", "time");
        query.set("orderBy/direction", "DESCENDING");
        query.set("limit", 1);

        JsonArray queryResult = queryFirestore(queryPath, query);

        if (queryResult.size() > 0)
        {
          livePower = queryResult[0]["document"]["fields"]["power"]["integerValue"].as<int>();
          Serial.printf("Power: %d\n", livePower);
        }
        else
        {
          Serial.println("No query result");
        }

        // Set LED attribute
        liveLEDCount = int(mapValue(livePower, 0, roundToHundred(maximumLivePower), 0, maxLiveLED));
        if (stopWheel == false)
        {
          motorSpeed = int(mapValue(livePower, 0, roundToHundred(maximumLivePower), 20, 50));
        }
        else
        {
          motorSpeed = 0;
        }
      }
    }

    ////////////////////////////////
    // Set NeoPixel color based on live data
    ////////////////////////////////
    if (!saveMarbleRequired && isLEDOn)
    {
      // Show LED animation
      if (millis() - lastTransitionMillis > transitionWait)
      {
        pixels.setPixelColor(CENTRE_LED, pixels.Color(255, 0, 0));
        if (liveLEDInitialising)
        {
          pixels.setPixelColor(CENTRE_LED + lastLEDCount, pixels.Color(232, 229, 88));
          pixels.show();
          lastLEDCount++;
          if (lastLEDCount == liveLEDCount)
          {
            liveLEDInitialising = false;
          }
        }
        else
        {
          if (liveLEDCount - lastLEDCount > 0)
          {
            pixels.setPixelColor(CENTRE_LED + lastLEDCount, pixels.Color(232, 229, 88));
            pixels.show();
            lastLEDCount++;
          }
          else if (liveLEDCount - lastLEDCount < 0)
          {
            pixels.setPixelColor(CENTRE_LED + lastLEDCount, pixels.Color(0, 0, 0));
            pixels.show();
            lastLEDCount--;
          }
          else
          {
            pixels.setPixelColor(CENTRE_LED + lastLEDCount, pixels.Color(232, 229, 88));
            pixels.show();
          }
        }
      }

      // Get history data and set LED
      if (millis() - dailyLEDMillis > 600000 || initialising)
      {
        dailyLEDMillis = millis();

        String queryPath = defaultPath;
        queryPath += "sensors/overall/";
        FirebaseJson query;

        query.set("select/fields/[0]/fieldPath", "today");
        query.set("select/fields/[1]/fieldPath", "time");
        query.set("from/collectionId", "history");
        query.set("from/allDescendants", false);
        query.set("orderBy/field/fieldPath", "time");
        query.set("orderBy/direction", "DESCENDING");
        query.set("limit", 1);

        JsonArray queryResult = queryFirestore(queryPath, query);

        if (queryResult.size() > 0)
        {
          float today = queryResult[0]["document"]["fields"]["today"]["doubleValue"].as<float>();
          Serial.printf("Today: %f\n", today);
          historyLEDCount = int(mapValue(today, 0, 2, 0, CENTRE_LED - 1));
          // historyLEDCount = int(map(today, 0, 2, 0, CENTRE_LED - 1));
        }
        else
        {
          Serial.println("No query result");
        }

        // Set LED attribute
        for (int i = 0; i < CENTRE_LED; i++)
        {
          pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        }
        for (int i = CENTRE_LED - 1; i > CENTRE_LED - historyLEDCount; i--)
        {
          pixels.setPixelColor(i, pixels.Color(232, 229, 88));
          pixels.show();
        }
      }
    }

    ////////////////////////////////
    // Set Motor Speed
    ////////////////////////////////
    if (!saveMarbleRequired)
    {
      myMotor->run(FORWARD);
      myMotor->setSpeed(motorSpeed);
    }

    ////////////////////////////////
    // Marble Saving Sequence
    ////////////////////////////////
    if (saveMarbleRequired == true)
    {
      if (isSavingFinished == true)
      {
        // wait for the last marble to drop
        if (millis() - lastSaveMillis > saveWait * 1.3)
        {
          Serial.println("Stopping saving sequence");
          myMotor->run(RELEASE);
          myMotor->run(FORWARD);
          myMotor->setSpeed(motorSpeed);
          saveMarbleRequired = false;
          saveAnimationInit = true;
          isSavingFinished = false;
          // LED Reset
          lastLEDCount = 0;
          pixels.clear();
          Serial.printf("liveLEDCount: %d\n", liveLEDCount);
          Serial.printf("historyLEDCount: %d\n", historyLEDCount);
          for (int i = CENTRE_LED - 1; i > CENTRE_LED - historyLEDCount; i--)
          {
            pixels.setPixelColor(i, pixels.Color(232, 229, 88));
            pixels.show();
          }
        }
      }
      else
      {
        if (saveAnimationInit == true)
        {
          myMotor->run(RELEASE);
          pixels.clear(); // Set all pixel colors to 'off'
          saveAnimationInit = false;
        }

        // Set LED animation
        if (millis() - lastSaveAnimationLEDMillis > transitionWait / 5 && isLEDOn == true)
        {
          pixels.clear();
          pixels.setPixelColor(saveAnimationLastLEDCount - 2, pixels.Color(232, 239, 247));
          pixels.setPixelColor(saveAnimationLastLEDCount - 1, pixels.Color(133, 186, 247));
          pixels.setPixelColor(saveAnimationLastLEDCount, pixels.Color(10, 120, 247));
          pixels.show();
          saveAnimationLastLEDCount--;
          if (saveAnimationLastLEDCount == 0)
          {
            saveAnimationLastLEDCount = NUMPIXELS;
          }
          lastSaveAnimationLEDMillis = millis();
        }

        myMotor->run(BACKWARD);
        myMotor->setSpeed(30);
        isTouched = digitalRead(33) == LOW ? true : false;
        if ((millis() - lastSaveMillis > saveWait) && (isTouched == true))
        {
          Serial.println("Touch sensor is pressed");
          savedMarble++;
          lastSaveMillis = millis();
          Serial.printf("savedMarble: %d / targetMarble: %d\n", savedMarble, targetMarble);
        }
        if (savedMarble >= targetMarble)
        {
          Serial.println("Saving is finished");
          isSavingFinished = true;
        }
      }
    }
    else
    {
      myMotor->run(FORWARD);
    }

    initialising = false;

    if (isLEDOn == false)
    {
      pixels.clear();
      pixels.show();
    }

    lastTransitionMillis = millis();

    // Regularly check for MQTT connection
    mqttClient.loop();
  }
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
    data.startDate += "Z"; // For ZULU format
    data.total = MQTTincoming["ENERGY"]["Total"];
    data.today = MQTTincoming["ENERGY"]["Today"];
    data.yesterday = MQTTincoming["ENERGY"]["Yesterday"];
    data.power = MQTTincoming["ENERGY"]["Power"];

    deviceList[deviceName] = data;

    FirebaseJson liveJson;
    String sensorLocation = "UCL/OPS/107";
    String sensorType = "EM";
    int livePower = data.power;
    String currentTime = getCurrentTime();
    String documentPath = defaultPath;

    liveJson.set("fields/lastUpdated/timestampValue", currentTime);

    /* Send the live data to Firestore
    Structure: device/{userUID}/sensors/{deviceName}/live/{autogeratedID}
    */
    if (Firebase.Firestore.patchDocument(&fbdo, PROJECT_ID, "", documentPath.c_str(), liveJson.raw(), "lastUpdated"))
    {
      documentPath += "sensors/";
      documentPath += deviceName;
      documentPath += "/";
      liveJson.clear();
      liveJson.set("fields/location/stringValue", sensorLocation);
      liveJson.set("fields/type/stringValue", sensorType);
      if (Firebase.Firestore.patchDocument(&fbdo, PROJECT_ID, "", documentPath.c_str(), liveJson.raw(), "location,type"))
      {
        documentPath += "live/";
        Serial.println(documentPath);
        liveJson.clear();
        liveJson.set("fields/power/integerValue", livePower);
        liveJson.set("fields/time/timestampValue", currentTime);
        if (Firebase.Firestore.createDocument(&fbdo, PROJECT_ID, "", documentPath.c_str(), liveJson.raw()))
        {
          // Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
          // print success message and time
          Serial.printf("Live Data: Success at %s\n", currentTime.c_str());
        }
        else
        {
          Serial.println(fbdo.errorReason());
        }
      }
      else
      {
        Serial.println(fbdo.errorReason());
      }
    }
    else
    {
      Serial.println(fbdo.errorReason());
    }
  }
}

void reconnect()
{
  while (!mqttClient.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect("DY_Device_Test"))
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

/* Send the live overall data to Firestore
    Structure: device/{userUID}/sensors/overall/live/{autoGeneratedID}
*/
String updateOverallLive()
{
  int powerSum = sumPower();
  String result = "Live Overall Data:";
  String currentTime = getCurrentTime();
  String liveOverallPath = defaultPath;
  liveOverallPath += "sensors/overall";
  Serial.printf("liveOverallPath: %s\n", liveOverallPath.c_str());
  // Check overall document exists, if not create one
  if (!Firebase.Firestore.getDocument(&fbdo, PROJECT_ID, "", liveOverallPath.c_str()))
  {
    FirebaseJson overallInitJson;
    overallInitJson.set("fields/created/timestampValue", currentTime);
    overallInitJson.set("fields/maxPower/integerValue", powerSum);
    Firebase.Firestore.createDocument(&fbdo, PROJECT_ID, "", liveOverallPath.c_str(), overallInitJson.raw());
    maximumLivePower = powerSum;
  }
  else
  {
    firestoreJSON.clear();
    DeserializationError error = deserializeJson(firestoreJSON, fbdo.payload().c_str());
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
    }
    else
    {
      // serializeJsonPretty(firestoreJSON, Serial);
      Serial.println("Deserialize JSON success");
    }
    int maxPower = firestoreJSON["fields"]["maxPower"]["integerValue"];
    // Serial.printf("maxPower: %d\n", maxPower);
    // Serial.printf("powerSum: %d\n", powerSum);
    FirebaseJson overallInitJson;
    overallInitJson.set("fields/lastUpdated/timestampValue", currentTime);
    if (maxPower < powerSum)
    {
      overallInitJson.set("fields/maxPower/integerValue", powerSum);

      maximumLivePower = powerSum;
    }

    bool result = Firebase.Firestore.patchDocument(&fbdo, PROJECT_ID, "", liveOverallPath.c_str(), overallInitJson.raw(), "lastUpdated,maxPower");
    if (!result)
    {
      Serial.println(fbdo.errorReason());
      Serial.println(fbdo.payload());
    }
  }

  liveOverallPath += "/live/";
  liveOverallJson.set("fields/time/timestampValue", currentTime);
  liveOverallJson.set("fields/devices/integerValue", deviceCount);
  liveOverallJson.set("fields/power/integerValue", powerSum);

  if (Firebase.Firestore.createDocument(&fbdo, PROJECT_ID, "", liveOverallPath.c_str(), liveOverallJson.raw()))
  {
    // Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
    result += "Success at";
    result += currentTime;
    return result;
  }
  else
  {
    return fbdo.errorReason();
  }
}

/* Send the live overall data to Firestore
    Structure: device/{userUID}/sensors/{deviceName}/history/{autoGeneratedID}
    Structure: device/{userUID}/sensors/overall/history/{autoGeneratedID}
*/
String updateHistory()
{
  String result = "History Data:";
  String currentTime = getCurrentTime();

  // Update history data for each device to Firestore
  for (auto const &pair : deviceList)
  {
    String historyPath = defaultPath;
    historyPath += "sensors/";
    historyPath += pair.first;
    historyPath += "/history/";

    FirebaseJson historyJson;
    historyJson.set("fields/time/timestampValue", currentTime);
    historyJson.set("fields/total/doubleValue", pair.second.total);
    historyJson.set("fields/today/doubleValue", pair.second.today);
    historyJson.set("fields/yesterday/doubleValue", pair.second.yesterday);
    historyJson.set("fields/startDate/timestampValue", pair.second.startDate);

    if (Firebase.Firestore.createDocument(&fbdo, PROJECT_ID, "", historyPath.c_str(), historyJson.raw()))
    {
      result += pair.first;
      result += ",";
      result += "Success at";
      result += currentTime;
    }
    else
    {
      result += "\n";
      result += pair.first;
      result += " Error:";
      result += fbdo.errorReason();
      result += "\n";
    }
  }

  // Update overall history data to Firestore
  String historyOverallPath = defaultPath;
  historyOverallPath += "sensors/overall/history/";

  historyOverallJson.set("fields/total/doubleValue", sumTotalUse());
  historyOverallJson.set("fields/today/doubleValue", sumTodayUse());
  historyOverallJson.set("fields/yesterday/doubleValue", sumYesterdayUse());
  historyOverallJson.set("fields/time/timestampValue", currentTime);

  if (Firebase.Firestore.createDocument(&fbdo, PROJECT_ID, "", historyOverallPath.c_str(), historyOverallJson.raw()))
  {
    result += "\n";
    result += "History Overall Success at";
    result += currentTime;
  }
  else
  {
    result += "\n";
    result += "History Overall Failed at";
    result += currentTime;
    result += "\nError:";
    result += fbdo.errorReason();
    result += "\n";
  }

  // Return result message
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

void rainbowFade2White(int wait, int rainbowLoops, int whiteLoops)
{
  int fadeVal = 0, fadeMax = 100;

  // Hue of first pixel runs 'rainbowLoops' complete loops through the color
  // wheel. Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to rainbowLoops*65536, using steps of 256 so we
  // advance around the wheel at a decent clip.
  for (uint32_t firstPixelHue = 0; firstPixelHue < rainbowLoops * 65536;
       firstPixelHue += 256)
  {

    for (int i = 0; i < pixels.numPixels(); i++)
    { // For each pixel in strip...

      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (strip.numPixels() steps):
      uint32_t pixelHue = firstPixelHue + (i * 65536L / pixels.numPixels());

      // strip.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the three-argument variant, though the
      // second value (saturation) is a constant 255.
      pixels.setPixelColor(i, pixels.gamma32(pixels.ColorHSV(pixelHue, 255,
                                                             255 * fadeVal / fadeMax)));
    }

    pixels.show();
    delay(wait);

    if (firstPixelHue < 65536)
    { // First loop,
      if (fadeVal < fadeMax)
        fadeVal++; // fade in
    }
    else if (firstPixelHue >= ((rainbowLoops - 1) * 65536))
    { // Last loop,
      if (fadeVal > 0)
        fadeVal--; // fade out
    }
    else
    {
      fadeVal = fadeMax; // Interim loop, make sure fade is at max
    }
  }

  for (int k = 0; k < whiteLoops; k++)
  {
    for (int j = 0; j < 256; j++)
    { // Ramp up 0 to 255
      // Fill entire strip with white at gamma-corrected brightness level 'j':
      pixels.fill(pixels.Color(0, 0, 0, pixels.gamma8(j)));
      pixels.show();
    }
    delay(1000); // Pause 1 second
    for (int j = 255; j >= 0; j--)
    { // Ramp down 255 to 0
      pixels.fill(pixels.Color(0, 0, 0, pixels.gamma8(j)));
      pixels.show();
    }
  }

  delay(500); // Pause 1/2 second
}

String getCurrentTime()
{
  time_t now;
  struct tm *timeinfo;
  char buffer[80];

  while (true)
  {
    time(&now);              // get the current time
    timeinfo = gmtime(&now); // convert the time to GMT (aka Zulu time)

    // If the year is not 1970, then we have a valid time
    if (timeinfo->tm_year != 70)
    {
      break;
    }
    Serial.println("Waiting for time to be set...");

    delay(1000); // Wait for a second before trying again
  }

  // Format the timeinfo struct into a string with the ISO 8601 / RFC 3339 format (Zulu Time)
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", timeinfo);

  return String(buffer);
}

JsonArray queryFirestore(String path, FirebaseJson query)
{
  firestoreJSON.clear();
  if (Firebase.Firestore.runQuery(&fbdo, PROJECT_ID, "", path, &query))
  {
    DeserializationError error = deserializeJson(firestoreJSON, fbdo.payload().c_str());

    // Test if parsing succeeds.
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
    }
    // Here's how you can access values in the document
    return firestoreJSON.as<JsonArray>();
  }
}

int roundToHundred(int num)
{
  return ((num + 99) / 100) * 100;
}

void checkButton1()
{
  unsigned long currentTime = millis();

  boolean buttonIsPressed = digitalRead(BUTTON1_PIN) == LOW; // Active LOW

  // Check for button state change and do debounce
  if (buttonIsPressed != Button1Pressed &&
      currentTime - Button1StateChangeTime > DebounceTime)
  {
    // Button state has changed
    Button1Pressed = buttonIsPressed;
    Button1StateChangeTime = currentTime;

    if (Button1Pressed)
    {
      // Button was just pressed
      isDebug = true;
    }
    else
    {
      // Button was just released
    }
  }

  if (Button1Pressed)
  {
    // Button is being held down
  }
}

void checkButton2()
{
  unsigned long currentTime = millis();

  boolean buttonIsPressed = digitalRead(BUTTON2_PIN) == LOW; // Active LOW

  // Check for button state change and do debounce
  if (buttonIsPressed != Button2Pressed &&
      currentTime - Button2StateChangeTime > DebounceTime)
  {
    // Button state has changed
    Button2Pressed = buttonIsPressed;
    Button2StateChangeTime = currentTime;

    if (Button2Pressed)
    {
      Serial.println("Button 2 is pressed");
      targetMarble = 2;
      savedMarble = 0;
      saveMarbleRequired = true;
      printf("savedMarble: %d / targetMarble: %d\n", savedMarble, targetMarble);
      // Button was just pressed
      if (isDebug)
      {
        motorSpeed += 5;
        // motorSpeed = 10;
        Serial.printf("Motor Speed: %d\n", motorSpeed);
      }
    }
    else
    {
      // Button was just released
    }
  }

  if (Button2Pressed)
  {
    // Button is being held down
  }
}

double calculateCost(float totalEnergyUse)
{
  return unitEnergyCost * totalEnergyUse;
}

bool checkMarbleSaving()
{
  targetMarble = round(calculateCost(sumTodayUse()) / targetCost);
  Serial.printf("targetMarble: %d\n", targetMarble);
  Serial.printf("savedMarble: %d\n", savedMarble);
  if (targetMarble >= 10)
  {
    Serial.println("Saving Marble : False (Too many marbles)");
    return false;
  }
  else if (savedMarble >= targetMarble)
  {
    Serial.println("Saving Marble : False (Saved enough)");
    return false;
  }
  else
  {
    Serial.println("Saving Marble : True");
    return true;
  }
}

float mapValue(float x, float minIn, float maxIn, float minOut, float maxOut)
{
  return (x - minIn) * (maxOut - minOut) / (maxIn - minIn) + minOut;
}
