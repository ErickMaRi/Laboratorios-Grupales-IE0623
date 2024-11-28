#include <Arduino_LSM9DS1.h>
#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include "model.h"

const int sampleRate = 100;  // Frecuencia de muestreo en Hz
const int numSamples = 119;  // Número de muestras necesarias para el modelo
const float accelerationThreshold = 2.5; // Umbral de movimiento significativo

// TensorFlow Lite
tflite::AllOpsResolver tflOpsResolver;
const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;

constexpr int tensorArenaSize = 8 * 1024;
byte tensorArena[tensorArenaSize] __attribute__((aligned(16)));

// Mapear índices de salida a movimientos
const char* MOVIMIENTOS[] = {"arriba", "jalar", "reves"};
#define NUM_MOVIMIENTOS (sizeof(MOVIMIENTOS) / sizeof(MOVIMIENTOS[0]))

float inputBuffer[numSamples * 3]; // Buffer para almacenar datos de entrada
int sampleIndex = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Inicializar IMU
  if (!IMU.begin()) {
    Serial.println("Error al inicializar la IMU!");
    while (1);
  }
  Serial.println("IMU inicializada.");

  // Inicializar modelo TensorFlow Lite
  tflModel = tflite::GetModel(model);
  if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Error: versión del modelo incompatible.");
    while (1);
  }

  tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize);
  tflInterpreter->AllocateTensors();
  tflInputTensor = tflInterpreter->input(0);
  tflOutputTensor = tflInterpreter->output(0);

  Serial.println("Modelo TensorFlow Lite cargado.");
}

void loop() {
  float ax, ay, az;

  // Leer datos de aceleración
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(ax, ay, az);

    // Guardar datos en el buffer
    inputBuffer[sampleIndex * 3 + 0] = ax;
    inputBuffer[sampleIndex * 3 + 1] = ay;
    inputBuffer[sampleIndex * 3 + 2] = az;
    sampleIndex++;

    // Si se llenó el buffer, realizar la clasificación
    if (sampleIndex >= numSamples) {
      sampleIndex = 0; // Reiniciar índice

      // Normalizar datos para el modelo
      for (int i = 0; i < numSamples * 3; i++) {
        tflInputTensor->data.f[i] = (inputBuffer[i] + 2.0) / 8.0; 
      }

      // Invocar el modelo
      if (tflInterpreter->Invoke() != kTfLiteOk) {
        Serial.println("Error al invocar el modelo.");
        return;
      }

       // Mostrar resultados de clasificación
       Serial.println("Movimiento detectado:");
       for (int i = 0; i < NUM_MOVIMIENTOS; i++) {
         Serial.print(MOVIMIENTOS[i]);
         Serial.print(": ");
         Serial.println(tflOutputTensor->data.f[i], 6);
       }
       Serial.println();
    // }
    
    // Determinar el movimiento con mayor probabilidad
      int maxIndex = -1;
      float maxScore = 0.0;

      for (int i = 0; i < NUM_MOVIMIENTOS; i++) {
        float score = tflOutputTensor->data.f[i];
        if (score > maxScore) {
          maxScore = score;
          maxIndex = i;
        }
      }

      // Mostrar el movimiento con mayor probabilidad si supera el umbral
      if (maxScore > 0.5 && maxIndex >= 0) {
        Serial.print("Movimiento detectado: ");
        Serial.println(MOVIMIENTOS[maxIndex]);
      } else {
        Serial.println("No se detectó un movimiento claro.");
      }
    }
  }

  // Control de la frecuencia de muestreo
  delay(1000 / sampleRate);
}
