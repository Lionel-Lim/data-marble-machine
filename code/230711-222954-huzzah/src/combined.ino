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
#define NUMPIXELS 40 // Popular NeoPixel ring size

#define PROJECT_ID "energycelab"

#define TOUCH_PIN 12

// Define Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Define JSON size
DynamicJsonDocument incomingLive(256);
DynamicJsonDocument MQTTincoming(512);

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

unsigned long liveDataMillis = 0;
unsigned long historyDataMillis = 0;
unsigned long testMillis = 0;
unsigned long liveLEDMillis = 0;
unsigned long dailyLEDMillis = 0;
unsigned long lastLEDMillis = 0;
int deviceCount = 0;
int ledWait = 250;
int liveLEDCount = 0;
int lastLEDCount = 0;
int liveLEDDefault = 19;
int touchValue = 0;
std::map<String, DeviceData> deviceList;
FirebaseJson liveOverallJson;
FirebaseJson historyOverallJson;
String defaultPath = "device/";
String userUID;
bool initialising = true;
bool liveLEDInitialising = true;
unsigned int livePower = 0;
unsigned int liveFactor = 15;
float todayUse = 0;
float todayFactor = 20;
QueryFilter query;
// TODO: Change the path dynamically in App or Web
String livePath = "/Data/42grHfoBn4hpVYyNBhven4G4Sgk2/UCL/OPS/107/EM/Live/overall";
String historyPath = "/Data/42grHfoBn4hpVYyNBhven4G4Sgk2/UCL/OPS/107/EM/History/Overall";

// Time Library Parameters
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

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

  // Set query for Firebase
  query.orderBy("$key");
  query.limitToLast(1);

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

  // Set NeoPixel color based on live data
  if (false)
  // if (Firebase.ready())
  {
    // pixels.clear(); // Set all pixel colors to 'off'
    pixels.setPixelColor(18, pixels.Color(255, 0, 0));
    // Get live Data and set LED
    if (millis() - liveLEDMillis > 15000 || initialising)
    {
      if (Firebase.RTDB.getJSON(&fbdo, livePath.c_str(), &query))
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
            // TEST: assign random int value from 0 to 300 to livePower
            livePower = int(random(0, 300));
            // Run motor
            // myMotor->run(FORWARD);
            Serial.print("Live Power: ");
            Serial.println(livePower);
            break; // Exit the loop after the first key-value pair.
          }
          liveLEDCount = (livePower / liveFactor);
          // print liveLEDCount and lastLEDCount
          Serial.print("liveLEDCount: ");
          Serial.println(liveLEDCount);
          Serial.print("lastLEDCount: ");
          Serial.println(lastLEDCount);
        }
        // pixels.show(); // Send the updated pixel colors to the hardware.
        liveLEDMillis = millis();
      }
      else
      {
        // Failed to get JSON data at defined database path, print out the error reason
        Serial.println(fbdo.errorReason());
      }
    }
    // Set LED animation
    if (millis() - lastLEDMillis > ledWait)
    {
      if (liveLEDInitialising)
      {
        pixels.setPixelColor(19 + lastLEDCount, pixels.Color(232, 229, 88));
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
          pixels.setPixelColor(19 + lastLEDCount, pixels.Color(232, 229, 88));
          pixels.show();
          lastLEDCount++;
        }
        else if (liveLEDCount - lastLEDCount < 0)
        {
          pixels.setPixelColor(19 + lastLEDCount, pixels.Color(0, 0, 0));
          pixels.show();
          lastLEDCount--;
        }
        else
        {
          pixels.setPixelColor(19 + lastLEDCount, pixels.Color(232, 229, 88));
          pixels.show();
        }
      }
      // Set motor speed
      int motorSpeed = int(map(lastLEDCount, 0, 20, 50, 150));
      // myMotor->setSpeed(motorSpeed);

      lastLEDMillis = millis();
    }

    // Get history data and set LED
    if (millis() - dailyLEDMillis > 600000 || initialising)
    {
      if (Firebase.RTDB.getJSON(&fbdo, historyPath.c_str(), &query))
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
            todayUse = value["today"];
            break; // Exit the loop after the first key-value pair.
          }
          for (int i = 0; i < 17; i++)
          {
            pixels.setPixelColor(i, pixels.Color(0, 0, 0));
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
        pixels.show();
        dailyLEDMillis = millis();
      }
      else
      {
        // Failed to get JSON data at defined database path, print out the error reason
        Serial.println(fbdo.errorReason());
      }
    }
  }

  initialising = false;

  // Read touch sensor value
  // if (millis() - testMillis > 1000)
  // {
  //   Serial.print("touch Value: ");
  //   Serial.println(touchRead(TOUCH_PIN));
  //   testMillis = millis();
  // }

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
    Firebase.Firestore.createDocument(&fbdo, PROJECT_ID, "", liveOverallPath.c_str(), overallInitJson.raw());
  }
  else
  {
    Serial.print("Overall document exists failed: ");
    Serial.println(fbdo.payload());
  }

  liveOverallPath += "/live/";
  liveOverallJson.set("fields/time/timestampValue", currentTime);
  liveOverallJson.set("fields/devices/integerValue", deviceCount);
  liveOverallJson.set("fields/power/integerValue", sumPower());

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
