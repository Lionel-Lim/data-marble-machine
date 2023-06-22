#include "secrets.h"
#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <PubSubClient.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>
#include <Adafruit_NeoPixel.h>

#define MQTT_SERVER "mqtt.cetools.org"
#define MQTT_TOPIC "UCL/OPS/107/EM/gosund/#"
// Firebase database address
#define DATABASE_URL "https://energycelab-default-rtdb.europe-west1.firebasedatabase.app"
// Digital IO pin connected to the NeoPixel attached to the board.
#define PIXEL_PIN 0

// Declare a NeoPixel object
Adafruit_NeoPixel singlePixel(1, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

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
bool booting = false;

void setup()
{
    Serial.begin(115200);

    //Initalise single NeoPixel
    singlePixel.begin();
    singlePixel.setBrightness(128); // Half brightness
    singlePixel.show();

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
    // While connecting to MQTT, light up red color
    singlePixel.setPixelColor(0, singlePixel.Color(255, 0, 0)); // Red color
    singlePixel.setPixelColor(1, singlePixel.Color(255, 0, 0)); // Red color
    singlePixel.show(); // Apply the color
    reconnect();
  }
  // Once MQTT is connected
  else if (booting == false) {
    booting = true;
    // Light up green color
    singlePixel.setPixelColor(0, singlePixel.Color(0, 255, 0)); // Green color
    singlePixel.setPixelColor(1, singlePixel.Color(0, 255, 0)); // Green color
    singlePixel.show(); // Apply the color
    delay(2000); // Wait for 2 seconds

    uint32_t startMillis = millis();
    Serial.print("Booting Started at: ");
    Serial.println(startMillis);
    while (millis() - startMillis < 5) { // Run for 5 seconds
      rainbow(5, singlePixel);
    }

    // Turn off LED
    singlePixel.clear(); // Clear the color
    singlePixel.show(); // Apply the color
    Serial.println("Booting Finished.");
  }
  client.loop();
}


void callback(char* topic, byte* payload, unsigned int length) {
    if (!booting) {
    return;
  }
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

// Rainbow cycle along whole neoPixel. Pass delay time (in ms) between frames.
void rainbow(int wait, Adafruit_NeoPixel neoPixel) {
  // Hue of first pixel runs 3 complete loops through the color wheel.
  // Color wheel has a range of 65536 but it's OK if we roll over, so
  // just count from 0 to 3*65536. Adding 256 to firstPixelHue each time
  // means we'll make 3*65536/256 = 768 passes through this outer loop:
  for(long firstPixelHue = 0; firstPixelHue < 3*65536; firstPixelHue += 256) {
    for(int i=0; i<neoPixel.numPixels(); i++) { // For each pixel in strip...
      // Offset pixel hue by an amount to make one full revolution of the
      // color wheel (range of 65536) along the length of the strip
      // (neoPixel.numPixels() steps):
      int pixelHue = firstPixelHue + (i * 65536L / neoPixel.numPixels());
      // neoPixel.ColorHSV() can take 1 or 3 arguments: a hue (0 to 65535) or
      // optionally add saturation and value (brightness) (each 0 to 255).
      // Here we're using just the single-argument hue variant. The result
      // is passed through neoPixel.gamma32() to provide 'truer' colors
      // before assigning to each pixel:
      neoPixel.setPixelColor(i, neoPixel.gamma32(neoPixel.ColorHSV(pixelHue)));
    }
    neoPixel.show(); // Update neoPixel with new contents
    delay(wait);  // Pause for a moment
  }
}