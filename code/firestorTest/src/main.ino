
/**
 * Created by K. Suwatchai (Mobizt)
 *
 * Email: k_suwatchai@hotmail.com
 *
 * Github: https://github.com/mobizt/Firebase-ESP-Client
 *
 * Copyright (c) 2023 mobizt
 *
 */

// This example shows how to run a query. This operation required Email/password, custom or OAUth2.0 authentication.

#include "secrets.h"
#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

#include <ArduinoJson.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID _WIFI_SSID
#define WIFI_PASSWORD _WIFI_PASSWORD

/* 2. Define the API Key */
#define API_KEY _API_KEY

/* 3. Define the project ID */
#define FIREBASE_PROJECT_ID "energycelab"

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL _USER_EMAIL
#define USER_PASSWORD _USER_PASSWORD

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long dataMillis = 0;
int count = 0;

DynamicJsonDocument doc(2048);

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

void setup()
{

    Serial.begin(115200);

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    multi.addAP(WIFI_SSID, WIFI_PASSWORD);
    multi.run();
#else
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

    Serial.print("Connecting to Wi-Fi");
    unsigned long ms = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
        if (millis() - ms > 10000)
            break;
#endif
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    /* Assign the project host and api key (required) */
    config.api_key = API_KEY;

    /* Assign the user sign in credentials */
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    // The WiFi credentials are required for Pico W
    // due to it does not have reconnect feature.
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    config.wifi.clearAP();
    config.wifi.addAP(WIFI_SSID, WIFI_PASSWORD);
#endif

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

#if defined(ESP8266)
    // In ESP8266 required for BearSSL rx/tx buffer for large data handle, increase Rx size as needed.
    fbdo.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 2048 /* Tx buffer size in bytes from 512 - 16384 */);
#endif

    // Limit the size of response payload to be collected in FirebaseData
    fbdo.setResponseSize(2048);

    Firebase.begin(&config, &auth);

    Firebase.reconnectWiFi(true);

    // You can use TCP KeepAlive in FirebaseData object and tracking the server connection status, please read this for detail.
    // https://github.com/mobizt/Firebase-ESP-Client#about-firebasedata-object
    // fbdo.keepAlive(5, 5, 1);
}

void loop()
{

    // Firebase.ready() should be called repeatedly to handle authentication tasks.

    if (Firebase.ready() && (millis() - dataMillis > 120000 || dataMillis == 0))
    {
        dataMillis = millis();

        String path = "/device/b0boEGdZJMYNdxMqi9tQ0UdEXMC3/sensors/overall/";

        Serial.print("Query a Firestore database... ");

        // If you have run the CreateDocuments example, the document b0 (in collection a0) contains the document collection c0, and
        // c0 contains the collections d?.

        // The following query will query at collection c0 to get the 3 documents in the payload result with descending order.

        // For the usage of FirebaseJson, see examples/FirebaseJson/BasicUsage/Create_Edit_Parse/Create_Edit_Parse.ino
        FirebaseJson query;

        query.set("select/fields/[0]/fieldPath", "power");
        query.set("select/fields/[1]/fieldPath", "time");

        query.set("from/collectionId", "live");
        query.set("from/allDescendants", false);
        query.set("orderBy/field/fieldPath", "time");
        query.set("orderBy/direction", "DESCENDING");
        query.set("limit", 1);

        // The consistencyMode and consistency arguments are not assigned
        // The consistencyMode is set to fb_esp_firestore_consistency_mode_undefined by default.
        // The arguments is the consistencyMode value, see the function description at
        // https://github.com/mobizt/Firebase-ESP-Client/tree/main/src#runs-a-query

        if (Firebase.Firestore.runQuery(&fbdo, FIREBASE_PROJECT_ID, "", path, &query))
        {
            Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
            DeserializationError error = deserializeJson(doc, fbdo.payload().c_str());

            // Test if parsing succeeds.
            if (error)
            {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.f_str());
                return;
            }
            // Here's how you can access values in the document
            JsonArray array = doc.as<JsonArray>();

            // Check if there are any documents in the array
            if (array.size() > 0)
            {
                JsonVariant v = array[0]; // Take the first document only

                JsonObject document = v["document"];
                String name = document["name"];
                int power = document["fields"]["power"]["integerValue"].as<int>();
                String timestampValue = document["fields"]["time"]["timestampValue"];
                String createTime = document["createTime"];
                String updateTime = document["updateTime"];
                String readTime = v["readTime"];

                // print values like name : value using printf
                Serial.printf("name : %s\n", name.c_str());
                Serial.printf("power : %d\n", power);
                Serial.printf("timestampValue : %s\n", timestampValue.c_str());
                Serial.printf("createTime : %s\n", createTime.c_str());
                Serial.printf("updateTime : %s\n", updateTime.c_str());
                Serial.printf("readTime : %s\n", readTime.c_str());
                Serial.println();
            }
            else
            {
                Serial.println("No documents in array");
            }
        }

        else
        {
            Serial.println(fbdo.errorReason());
        }
    }
}