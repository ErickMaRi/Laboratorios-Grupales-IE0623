#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

// Configuración de la red WiFi
const char* ssid = "Apartamento JM";         
const char* password = "JMQVLDFM.07"; 

// Configuracion ThingsBoard
const String accessToken = "52DSeWqWFQaWsEJ7W5uK"; // Device access token
const String serverUrl = "https://iot.eie.ucr.ac.cr/api/v1/" + accessToken + "/attributes"; // endpoint

WiFiClientSecure client; // Cliente seguro para HTTPS

void setup() {
  Serial.begin(115200);

  // Configuración de WiFi
  WiFi.begin(ssid, password);
  Serial.println("Conectando a WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConectado a WiFi");

  // Configuración del cliente HTTPS
  client.setInsecure(); // Ignorar la verificación de certificados SSL
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient https;

    Serial.println("Iniciando solicitud GET...");
    https.begin(client, serverUrl); // Configurar la URL del servidor con WiFiClientSecure para HTTPS
    int httpResponseCode = https.GET(); // Realizar la solicitud GET

    if (httpResponseCode > 0) {
      // Si la solicitud fue exitosa
      String payload = https.getString();
      Serial.println("Respuesta del servidor:");
      Serial.println(payload);
    } else {
      // Si hay un error en la solicitud
      Serial.print("Error en la solicitud GET. Código: ");
      Serial.println(httpResponseCode);
    }

    https.end(); // Cerrar la conexión
  } else {
    Serial.println("No hay conexión WiFi");
  }

  delay(10000); // Esperar 10 segundos antes de la siguiente solicitud
}
