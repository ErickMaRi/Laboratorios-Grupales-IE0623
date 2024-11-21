#include <Arduino_LSM9DS1.h>

const int buttonPin = 2;  // Pin del botón
const int sampleRate = 100;  // Frecuencia de muestreo en Hz

bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; // 50 ms para debounce
bool saveRequested = false;

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);

  if (!IMU.begin()) {
    Serial.println("Error al inicializar la IMU!");
    while (1);
  }
  Serial.println("IMU inicializada.");
}

void loop() {
  // Lectura de la aceleración
  if (IMU.accelerationAvailable()) {
    float ax, ay, az;
    IMU.readAcceleration(ax, ay, az);

    Serial.print(ax);
    Serial.print(",");
    Serial.print(ay);
    Serial.print(",");
    Serial.println(az);
  }

  // Ya no leemos con el botón del arduino
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading == LOW && lastButtonState == HIGH && !saveRequested) {

      saveRequested = true;
      Serial.println("Botón presionado"); 
    }
    if (reading == HIGH && saveRequested) {
      saveRequested = false;
    }
  }

  lastButtonState = reading;

  // Control de la frecuencia de muestreo
  delay(1000 / sampleRate);
}
