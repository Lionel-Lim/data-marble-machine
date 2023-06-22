#include "secrets.h"
#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <PubSubClient.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>
// #include </Users/dylim/Library/Arduino15/packages/firebeetle32/hardware/esp32/0.1.1/libraries/SD/src/SD.h>

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

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long dataMillis = 0;
int count = 0;
int led = LED_BUILTIN;

void setup()
{
    Serial.begin(115200);

    // set LED(13) to be an output pin
    pinMode(led, OUTPUT);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println("Connecting to Wi-Fi");
    // Try to connect WiFi, LED(13) blinking shows the progress
    while (WiFi.status() != WL_CONNECTED)
    {
      digitalWrite(led, HIGH);
      delay(1000);
      Serial.println("Connecting to WiFi...");
      digitalWrite(led, LOW);
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

    client.setServer(MQTT_SERVER, 1883);
    client.setCallback(callback);
    client.setBufferSize(512);
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("Message is:");
  char msg[length+1];
  memcpy (msg, payload, length);
  msg[length] = '\0';
  Serial.printf("Message arrived: %s\n", msg);

  String path = "/UsersData/";
  path += auth.token.uid.c_str();
  path += "/test/data";

  FirebaseJson json;
  json.set("topic", topic);
  json.set("value", msg);
  json.set("timestamp", Firebase.getCurrentTime());

  Serial.printf("Push data... %s\n", Firebase.pushJSON(fbdo, path, json) ? "ok" : fbdo.errorReason().c_str());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("DY_ESP32")) {
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

// void loop()
// {
//   if (millis() - dataMillis > 5000 && Firebase.ready())
//   {
//     dataMillis = millis();
//     String path = "/UsersData/";
//     path += auth.token.uid.c_str();
//     path += "/test/data";

//     FirebaseJson json;
//     json.set("value", count++);
//     json.set("timestamp", Firebase.getCurrentTime());

//     Serial.printf("Push data... %s\n", Firebase.pushJSON(firebaseData, path, json) ? "ok" : firebaseData.errorReason().c_str());
//   }
// }
