#include <WiFiEspClient.h>
#include <WiFiEsp.h>
#include "SoftwareSerial.h"
#include <ThingsBoard.h>
#include <Base64.h>
#include <PubSubClient.h>

#define WIFI_AP "Mey"
#define WIFI_PASSWORD "Alvarado19"

#define TOKEN "06hyrjcbtwq2n5lrxzsv"
char thingsboardServer[] = "https://iot.eie.ucr.ac.cr/";

WiFiEspClient espClient;
PubSubClient mqttClient(espClient);
ThingsBoard tb(mqttClient);  // Use PubSubClient with ThingsBoard

SoftwareSerial soft(2, 3); // RX, TX

int status = WL_IDLE_STATUS;
unsigned long lastSend;

void setup() {
  // initialize serial for debugging
  Serial.begin(9600);
  InitWiFi();
  lastSend = 0;
}

void loop() {
  status = WiFi.status();
  if (status != WL_CONNECTED) {
    while (status != WL_CONNECTED) {
      Serial.print("Attempting to connect to WPA SSID: ");
      Serial.println(WIFI_AP);
      // Connect to WPA/WPA2 network
      status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);
      delay(500);
    }
    Serial.println("Connected to AP");
  }

  if (!tb.connected()) {
    reconnect();
  }

  getCalendarData();

  tb.loop();
}

void getCalendarData() {
  if (tb.connected()) {
    String jsonData = tb.receiveTelemetry();
    if (jsonData.length() > 0) {
      Serial.println("Received telemetry data:");
      Serial.println(jsonData);

      // Assuming the base64 data is in the "file" key, let's extract and decode it
      int startIndex = jsonData.indexOf("\"file\":\"") + 8;  // Start after the "file":" part
      int endIndex = jsonData.indexOf("\"", startIndex);  // Find the closing quote

      if (startIndex != -1 && endIndex != -1) {
        String base64File = jsonData.substring(startIndex, endIndex);  // Extract the base64 string

        Serial.println("Decoding base64 data...");
        
        // Base64 decoding
        int decodedLength = base64_decoded_length(base64File.c_str());  // Calculate decoded data length
        byte decodedData[decodedLength];  // Create a buffer to hold the decoded data

        int result = base64_decode(decodedData, base64File.c_str(), base64File.length());  // Decode the base64

        if (result == decodedLength) {
          Serial.println("Base64 decoding successful.");
          
          // Process the decoded CSV data here (for example, print it)
          String decodedStr = String((char*)decodedData);
          Serial.println("Decoded CSV Data:");
          Serial.println(decodedStr);
        } else {
          Serial.println("Error decoding base64 data.");
        }
      } else {
        Serial.println("No file found in telemetry.");
      }
    } else {
      Serial.println("No new data.");
    }
  } else {
    Serial.println("Not connected to ThingsBoard.");
  }
}

void InitWiFi() {
  // initialize serial for ESP module
  soft.begin(9600);
  // initialize ESP module
  WiFi.init(&soft);
  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    while (true);
  }

  Serial.println("Connecting to AP ...");
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(WIFI_AP);
    status = WiFi.begin(WIFI_AP, WIFI_PASSWORD);
    delay(500);
  }
  Serial.println("Connected to AP");
}

void reconnect() {
  while (!tb.connected()) {
    Serial.print("Connecting to ThingsBoard node ...");
    if (tb.connect(thingsboardServer, TOKEN)) {
      Serial.println("[DONE]");
    } else {
      Serial.print("[FAILED]");
      Serial.println(" : retrying in 5 seconds");
      delay(5000);
    }
  }
}
