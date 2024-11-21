#include <Arduino_LSM9DS1.h>
#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include "model.h"

const int sampleRate = 100;  // Frecuencia de muestreo en Hz
const int numSamples = 119;  // Número de muestras necesarias para el modelo

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

unsigned long previousMicros = 0; // Para control de tiempo

// Variables para almacenar las últimas salidas del modelo
float latest_prob1 = 0.0;
float latest_prob2 = 0.0;
float latest_prob3 = 0.0;
const char* latest_movementLabel = "none";
float latest_infer_time = 0.0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Inicializar IMU
  if (!IMU.begin()) {
    Serial.println("IMU inicializada.");
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
  unsigned long currentMicros = micros();
  unsigned long interval = 1000000 / sampleRate; // Intervalo en microsegundos (10 ms para 100 Hz)

  if (currentMicros - previousMicros >= interval) {
    previousMicros += interval;

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

        // Medir el tiempo de inferencia
        unsigned long startTime = micros();

        // Normalizar datos para el modelo
        for (int i = 0; i < numSamples * 3; i++) {
          tflInputTensor->data.f[i] = (inputBuffer[i] + 4.0) / 8.0;
        }

        // Invocar el modelo
        if (tflInterpreter->Invoke() != kTfLiteOk) {
          Serial.println("Error al invocar el modelo.");
          // No usar 'return' para evitar detener el loop
        }

        // Medir el tiempo de inferencia
        unsigned long endTime = micros();
        latest_infer_time = (endTime - startTime) / 1000.0; // Tiempo en milisegundos

        // Obtener probabilidades de cada clase
        latest_prob1 = tflOutputTensor->data.f[0];
        latest_prob2 = tflOutputTensor->data.f[1];
        latest_prob3 = tflOutputTensor->data.f[2];

        // Encontrar el movimiento con mayor probabilidad
        int movimientoIndex = 0;
        float maxProb = latest_prob1;
        if (latest_prob2 > maxProb) {
          maxProb = latest_prob2;
          movimientoIndex = 1;
        }
        if (latest_prob3 > maxProb) {
          maxProb = latest_prob3;
          movimientoIndex = 2;
        }

        latest_movementLabel = MOVIMIENTOS[movimientoIndex];

        // Determinar si se pudo mantener 100 Hz
        bool maintainRate = latest_infer_time <= (1000.0 / sampleRate); // 10 ms para 100 Hz
        if (!maintainRate) {
          // Si no se pudo mantener, infer_time se registró
        }
      }

      // Enviar los datos del acelerómetro junto con las últimas salidas del modelo
      // Esto asegura que los datos se envíen a 100 Hz
      // Incluso si el modelo no ha sido invocado recientemente, se envían las últimas salidas disponibles
      Serial.print(ax, 4);
      Serial.print(",");
      Serial.print(ay, 4);
      Serial.print(",");
      Serial.print(az, 4);
      Serial.print(",");
      Serial.print(latest_prob1, 4);
      Serial.print(",");
      Serial.print(latest_prob2, 4);
      Serial.print(",");
      Serial.print(latest_prob3, 4);
      Serial.print(",");
      Serial.print(latest_movementLabel);
      Serial.print(",");
      Serial.println((latest_infer_time <= (1000.0 / sampleRate)) ? "0.000" : String(latest_infer_time, 3).c_str());
    }
  }
}
