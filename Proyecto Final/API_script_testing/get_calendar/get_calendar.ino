#include <ESP8266WiFi.h>
#include <ThingsBoard.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <Base64.h>  // Include the Base64 library

#define WIFI_AP "Mey"
#define WIFI_PASSWORD "Alvarado19"
#define TOKEN "06hyrjcbtwq2n5lrxzsv"
#define THINGSBOARD_HOST "https://iot.eie.ucr.ac.cr/"

// Setup ThingsBoard client and WiFi
WiFiClient wifiClient;
ThingsBoard tb(wifiClient);

int status = WL_IDLE_STATUS;
unsigned long lastFetchTime = 0;
const long fetchInterval = 60000; // Fetch data every 60 seconds 
void setup() {
  Serial.begin(9600);
  delay(10);
  initWiFi();
  lastFetchTime = millis();
}

void loop() {
  // Check if we need to fetch data from ThingsBoard
  if (millis() - lastFetchTime > fetchInterval) {
    fetchFileFromThingsBoard();
    lastFetchTime = millis();
  }

  tb.loop();  // Keep ThingsBoard communication active
}

void initWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_AP, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
}

void fetchFileFromThingsBoard() {
  if (WiFi.status() != WL_CONNECTED) {
    reconnectToThingsBoard();
  }

  HTTPClient http;
  String url = String("http://") + THINGSBOARD_HOST + "/api/v1/" + TOKEN + "/telemetry";

  http.begin(url);  // HTTP request to ThingsBoard
  int httpCode = http.GET();  // Send GET request

  if (httpCode == HTTP_CODE_OK) {
    String jsonData = http.getString();  // Get the response body
    Serial.println("Data received from ThingsBoard:");

    // Parse the JSON response to get base64-encoded file
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, jsonData);
    
    if (error) {
      Serial.print("Error parsing JSON: ");
      Serial.println(error.c_str());
      return;
    }

    // Check if the file is in the response
    if (doc.containsKey("file")) {
      String base64File = doc["file"].as<String>();
      decodeAndProcessCSV(base64File);
    } else {
      Serial.println("No file found in the response.");
    }
  } else {
    Serial.print("Failed to fetch data. HTTP error code: ");
    Serial.println(httpCode);
  }

  http.end();  // Close HTTP connection
}

void reconnectToThingsBoard() {
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Connecting to WiFi...");
    delay(500);
  }
  Serial.println("Connected to WiFi");
}

void decodeAndProcessCSV(String base64File) {
  // Create a mutable char array for the base64-encoded string
  char base64CharArray[base64File.length() + 1];  // +1 for the null terminator
  base64File.toCharArray(base64CharArray, sizeof(base64CharArray));

  // Get the decoded length
  int decodedLength = Base64.decode(NULL, base64CharArray, base64File.length());

  // Allocate memory for decoded data
  char* decodedData = new char[decodedLength + 1];  // +1 for the null terminator

  // Perform the decoding
  Base64.decode(decodedData, base64CharArray, base64File.length());

  // Now, decodedData contains the CSV file (as char[])
  Serial.println("Decoded CSV content:");
  for (int i = 0; i < decodedLength; i++) {
    Serial.write(decodedData[i]);
  }
  Serial.println();  // Line break for clarity

  // Clean up
  delete[] decodedData;
}
