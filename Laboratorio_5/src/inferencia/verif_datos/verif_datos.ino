#include <Arduino_LSM9DS1.h>
#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include "model.h"

const int sampleRate = 100;  // Frecuencia de muestreo en Hz
const int numSamples = 119;  // Número de muestras necesarias para el modelo
const int stride = 10;       // Desplazamiento de la ventana en cada inferencia

// TensorFlow Lite
tflite::AllOpsResolver tflOpsResolver;
const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;

constexpr int tensorArenaSize = 16 * 1024; 
byte tensorArena[tensorArenaSize] __attribute__((aligned(16)));

// Indices de salidas
const char* MOVIMIENTOS[] = {"arriba", "jalar", "reves"};
#define NUM_MOVIMIENTOS (sizeof(MOVIMIENTOS) / sizeof(MOVIMIENTOS[0]))

float inputBuffer[numSamples * 3]; // Buffer para almacenar datos de entrada
int sampleIndex = 0;

// Buffer circular para inferencia deslizante
float slidingBuffer[numSamples * 3];
int slidingIndex = 0;

// Variables para almacenar las últimas salidas del modelo
float latest_prob1 = 0.0;
float latest_prob2 = 0.0;
float latest_prob3 = 0.0;
const char* latest_movementLabel = "none";
float latest_infer_time = 0.0;

unsigned long previousMicros = 0; // Para el tiempo

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Inicializar la IMU
  if (!IMU.begin()) {
    Serial.println("IMU initialization failed!");
    while (1);
  }
  Serial.println("IMU initialized.");

  // Inicializar modelo TensorFlow Lite
  tflModel = tflite::GetModel(model);
  if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model version does not match Schema.");
    while (1);
  }

  tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize);
  tflInterpreter->AllocateTensors();
  tflInputTensor = tflInterpreter->input(0);
  tflOutputTensor = tflInterpreter->output(0);

  Serial.println("TensorFlow Lite model loaded.");

  // Inicializar el buffer deslizante con ceros
  for (int i = 0; i < numSamples * 3; i++) {
    slidingBuffer[i] = 0.0;
  }
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

      // Guardar datos en el buffer principal
      inputBuffer[sampleIndex * 3 + 0] = ax;
      inputBuffer[sampleIndex * 3 + 1] = ay;
      inputBuffer[sampleIndex * 3 + 2] = az;
      sampleIndex++;

      // Guardar datos en el buffer deslizante
      for (int i = 0; i < 3; i++) {
        slidingBuffer[slidingIndex * 3 + i] = inputBuffer[sampleIndex * 3 + i - stride * 3 + i];
      }
      slidingIndex = (slidingIndex + stride) % (numSamples);

      // Realizar inferencia cada 'stride' muestras
      if (sampleIndex >= stride) {
        sampleIndex = 0; // Reiniciar índice para el buffer principal

        // Copiar datos del buffer deslizante a la entrada del modelo
        for (int i = 0; i < numSamples * 3; i++) {
          tflInputTensor->data.f[i] = (slidingBuffer[i] + 4.0) / 8.0; // Normalización
        }

        // Medir el tiempo de inferencia
        unsigned long startTime = micros();

        // Invocar el modelo
        if (tflInterpreter->Invoke() != kTfLiteOk) {
          Serial.println("Error invoking the model.");
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

        // Enviar los datos del acelerómetro junto con las últimas salidas del modelo
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
        if (maintainRate) {
          Serial.println("0.000");
        } else {
          Serial.println(String(latest_infer_time, 3).c_str());
        }
      }
    }
  }
}
