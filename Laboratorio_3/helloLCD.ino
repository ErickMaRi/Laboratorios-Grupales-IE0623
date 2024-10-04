#include <Adafruit_GFX.h>      // Librería para funciones gráficas
#include <Adafruit_PCD8544.h>  // Librería para la pantalla PCD8544
#include <SPI.h>               // Librería para comunicación SPI
#include <math.h>              // Librería para funciones matemáticas

// -----------------------------
// Definición de Pines y Constantes
// -----------------------------


const int NUM_CHANNELS = 4;

// Pines para controlar la transmisión serial y modo AC/DC
const int USART_SWITCH_PIN = 7; // Pin para controlar la transmisión serial
const int MODE_SWITCH_PIN = 6;  // Pin para seleccionar entre AC y DC

// Pines para alarmas (LEDs)
const int ALARM_PINS[NUM_CHANNELS] = {0, 1, 2, 3, 4};

// Pines para ADC (A0 - A3)
const int ADC_PINS[NUM_CHANNELS] = {A0, A1, A2, A3};

// Pines para la pantalla LCD PCD8544
#define RST_PIN 8
#define CE_PIN 10
#define DC_PIN 9
#define DIN_PIN 11
#define CLK_PIN 13


// Parámetros de voltaje
const float V_IN_MIN = -24.0; // Voltios en las entradas analógicas
const float V_IN_MAX = 24.0;  // Voltios
const float V_ADC_MIN = 0.0;  // Voltios
const float V_ADC_MAX = 5.0;  // Voltios

// Límites para activar alarmas
const float ALARM_THRESHOLD_LOW = -20.0; // Voltios
const float ALARM_THRESHOLD_HIGH = 20.0; // Voltios

// Configuración del puerto serial
const unsigned long SERIAL_BAUD_RATE = 9600;

// Variables para almacenar los voltajes reales
float realVoltages[NUM_CHANNELS] = {0.0, 0.0, 0.0, 0.0};

// Variables para almacenar el estado de las alarmas
bool alarms[NUM_CHANNELS] = {false, false, false, false};

// Temporizador para medir voltajes AC
const int NUM_SAMPLES = 100; // Número de muestras para cálculo RMS
float samples[NUM_SAMPLES][NUM_CHANNELS]; // Muestras para cada canal
int sampleIndex = 0;
unsigned long sampleInterval = 10; // Intervalo entre muestras en ms
unsigned long lastSampleTime = 0;

// Temporizador para envío serial
unsigned long lastSerialSend = 0;
const long serialInterval = 1000; // Intervalo de 1 segundo

// Variables para manejar el modo AC/DC
bool currentMode = false; // False: DC, True: AC

// Variables para manejar el USART
bool serialEnabled = false;

// Parámetro para controlar el modo de prueba
const bool USE_TEST_MODE = true; // Modo de prueba activado

// -----------------------------
// Clase DisplayManager
// -----------------------------

class DisplayManager {
  public:
    // Constructor de la clase
    DisplayManager(uint8_t clk, uint8_t din, uint8_t dc, uint8_t ce, uint8_t rst)
      : display(clk, din, dc, ce, rst),                 // Inicializa la pantalla con los pines proporcionados
        lastDisplayedVoltages{0.0, 0.0, 0.0, 0.0},        // Almacena los últimos voltajes mostrados
        lastDisplayedModes{false, false, false, false},   // Almacena los últimos modos (AC/DC) mostrados
        lastDisplayedAlarms{false, false, false, false} {} // Almacena las últimas alarmas mostradas

    // Función para inicializar la pantalla
    void initialize() {
      display.begin();                    // Inicia la pantalla
      display.setContrast(60);            // Ajusta el contraste (puedes cambiar el valor si es necesario)
      display.clearDisplay();             // Limpia la pantalla
      display.display();                  // Envía el cambio a la pantalla

      // Muestra un mensaje de bienvenida
      display.setTextSize(1);             // Tamaño pequeño del texto
      display.setTextColor(BLACK);        // Color del texto (negro)
      display.setCursor(0, 0);            // Posición inicial del cursor
      display.println("Voltimetro de 4 Canales");
      display.println("Inicializando...");
      display.display();                   // Muestra el texto en la pantalla
      delay(2000);                         // Espera 2 segundos para que el usuario lea el mensaje
      display.clearDisplay();              // Limpia la pantalla nuevamente
      display.display();                   // Envía el cambio a la pantalla
    }

    // Función para mostrar los voltajes en la pantalla
    void displayVoltages(float voltages[], bool isAC[]) {
      bool needsUpdate = false; // Bandera para saber si se necesita actualizar la pantalla

      // Revisa si los voltajes o modos han cambiado desde la última vez que se mostraron
      for(int i = 0; i < NUM_CHANNELS; i++) {
        if(abs(voltages[i] - lastDisplayedVoltages[i]) > 0.01 || isAC[i] != lastDisplayedModes[i]) {
          needsUpdate = true;                        // Si hay cambios, se necesita actualizar
          lastDisplayedVoltages[i] = voltages[i];    // Actualiza el voltaje almacenado
          lastDisplayedModes[i] = isAC[i];           // Actualiza el modo almacenado
        }
      }

      if(needsUpdate) { // Si se necesita actualizar la pantalla
        const int baseY = 0;             // Posición vertical inicial
        const int lineHeight = 12;       // Altura de cada línea de texto (12 píxeles)
        const int voltagesAreaHeight = NUM_CHANNELS * lineHeight;

        // Limpia la sección donde se muestran los voltajes
        display.fillRect(0, baseY, 84, voltagesAreaHeight, WHITE);
        display.setTextSize(1);          // Tamaño pequeño del texto
        display.setTextColor(BLACK);      // Color del texto (negro)

        // Recorre cada canal para mostrar su voltaje y modo
        for(int i = 0; i < NUM_CHANNELS; i++) {
          int yPosition = baseY + i * lineHeight; // Calcula la posición vertical de la línea
          display.setCursor(0, yPosition);         // Posiciona el cursor
          String mode = isAC[i] ? "AC" : "DC";      // Determina el modo (AC o DC)

          // Escribe el texto en el formato: CH1: 12.34 V DC
          display.print("CH");
          display.print(i+1);
          if (voltages[i] < 0) {
            display.print(":");
          } else {
            display.print(": ");
          }
          display.print(voltages[i], 2); // Muestra el voltaje con 2 decimales
          display.print(" V");
          display.println(mode);
        }

        display.display(); // Envía los cambios a la pantalla
      }
    }

    // Función para mostrar las alarmas en la pantalla
    void displayAlarms(bool alarms[]) {
      bool needsUpdate = false; // Bandera para saber si se necesita actualizar la pantalla

      // Revisa si las alarmas han cambiado desde la última vez que se mostraron
      for(int i = 0; i < NUM_CHANNELS; i++) {
        if(alarms[i] != lastDisplayedAlarms[i]) {
          needsUpdate = true;                       // Si hay cambios, se necesita actualizar
          lastDisplayedAlarms[i] = alarms[i];      // Actualiza la alarma almacenada
        }
      }

      if(needsUpdate) { // Si se necesita actualizar la pantalla
        const int voltagesAreaHeight = NUM_CHANNELS * 12; // Altura de la sección de voltajes (12 píxeles por canal)
        const int alarmY = voltagesAreaHeight + 2; // Posición vertical donde empieza la sección de alarmas

        // Limpia la sección de alarmas
        display.fillRect(0, alarmY, 84, 16, WHITE); // 16 píxeles de altura para alarmas
        display.setTextSize(1);           // Tamaño pequeño del texto
        display.setTextColor(BLACK);       // Color del texto (negro)
        display.setCursor(0, alarmY);       // Posiciona el cursor

        bool anyAlarm = false; // Bandera para saber si hay alguna alarma activa
        String alarmStr = "";  // Cadena de texto para mostrar las alarmas activas

        // Recorre cada canal para ver si hay alarmas
        for(int i = 0; i < NUM_CHANNELS; i++) {
          if(alarms[i]) { // Si hay alarma en el canal i
            if(anyAlarm) {
              alarmStr += ", CH" + String(i+1); // Añade el canal a la cadena de alarmas
            } else {
              alarmStr += "CH" + String(i+1);
              anyAlarm = true; // Marca que hay al menos una alarma
            }
          }
        }

        if(anyAlarm) { // Si hay alguna alarma activa
          alarmStr = "Alerta: " + alarmStr; // Prepara el mensaje de alerta
          display.println(alarmStr);        // Muestra el mensaje en la pantalla
        } else {
          display.println("Sin alertas");    // Si no hay alarmas, muestra este mensaje
        }

        display.display(); // Envía los cambios a la pantalla
      }
    }

    // Función para limpiar la sección de alarmas
    void clearAlarms() {
      const int voltagesAreaHeight = NUM_CHANNELS * 12; // Altura de la sección de voltajes
      const int alarmY = voltagesAreaHeight + 2; // Posición vertical donde empieza la sección de alarmas

      // Limpia la sección de alarmas
      display.fillRect(0, alarmY, 84, 16, WHITE);
      display.setTextSize(1);           // Tamaño pequeño del texto
      display.setTextColor(BLACK);      // Color del texto (negro)
      display.setCursor(0, alarmY);      // Posiciona el cursor
      display.println("Sin alertas");    // Muestra el mensaje "Sin alertas"
      display.display();                 // Envía los cambios a la pantalla

      // Actualiza el estado interno para que no se muestren alarmas
      for(int i = 0; i < NUM_CHANNELS; i++) {
        lastDisplayedAlarms[i] = false;
      }
    }

  private:
    Adafruit_PCD8544 display;             // Objeto para controlar la pantalla PCD8544
    float lastDisplayedVoltages[NUM_CHANNELS];        // Almacena los últimos voltajes mostrados
    bool lastDisplayedModes[NUM_CHANNELS];           // Almacena los últimos modos (AC/DC) mostrados
    bool lastDisplayedAlarms[NUM_CHANNELS];          // Almacena las últimas alarmas mostradas
};

// -----------------------------
// Instanciación de Objetos
// -----------------------------

// Creación del objeto DisplayManager con los pines definidos
DisplayManager displayManager(CLK_PIN, DIN_PIN, DC_PIN, CE_PIN, RST_PIN);

// -----------------------------
// Funciones Auxiliares
// -----------------------------

/*
  Función para escalar el valor leído del ADC al rango real de voltaje.
  @param adcValue: Valor leído del ADC (0 - 1023)
  @return Voltaje real en el rango [-24V, 24V]
*/
float scaleVoltage(int adcValue) {
  // Escala el valor del ADC a voltaje (0-5V)
  float voltage = (adcValue / 1023.0) * V_ADC_MAX; // [0,5]V

  // Mapear de [0,5]V a [-24,24]V
  float scaledVoltage = (voltage - (V_ADC_MAX / 2)) * (V_IN_MAX / (V_ADC_MAX / 2));
  return scaledVoltage;
}

/*
  Función para calcular el voltaje RMS a partir de muestras.
  @param samples: Array de muestras de voltaje
  @param numSamples: Número de muestras
  @return Voltaje RMS
*/
float calculateRMS(float samplesArray[], int numSamples) {
  float sumSquares = 0.0;
  for(int i = 0; i < numSamples; i++) {
    sumSquares += samplesArray[i] * samplesArray[i];
  }
  float meanSquares = sumSquares / numSamples;
  return sqrt(meanSquares);
}

/*
  Función para manejar las alarmas y actualizar el estado de los LEDs.
  @param voltages: Array de voltajes de cada canal
*/
void handleAlarms(float voltages[]) {
  for(int i = 0; i < NUM_CHANNELS; i++) {
    if(voltages[i] < ALARM_THRESHOLD_LOW || voltages[i] > ALARM_THRESHOLD_HIGH) {
      alarms[i] = true;
      digitalWrite(ALARM_PINS[i], HIGH); // Enciende el LED de alarma
    } else {
      alarms[i] = false;
      digitalWrite(ALARM_PINS[i], LOW);  // Apaga el LED de alarma
    }
  }
}

/*
  Función para leer el estado del switch de modo AC/DC.
  @return True si está en modo AC, False si está en modo DC
*/
bool readModeSwitch() {
  int switchState = digitalRead(MODE_SWITCH_PIN);
  return (switchState == HIGH) ? true : false; // True: AC, False: DC
}

/*
  Función para verificar si la transmisión serial está habilitada.
  @return True si la transmisión está habilitada, False de lo contrario
*/
bool isSerialEnabledFunc() {
  int switchState = digitalRead(USART_SWITCH_PIN);
  return (switchState == HIGH) ? true : false; // True: Enabled, False: Disabled
}

// -----------------------------
// Función setup
// -----------------------------

/*
  La función setup() se ejecuta una vez al inicio.
  Aquí inicializamos la pantalla, los pines, y configuramos la comunicación serial.
*/
void setup() {
  // Inicializar la pantalla
  displayManager.initialize();

  // Configuración de pines de alarma como salida (independientemente del modo)
  for(int i = 0; i < NUM_CHANNELS; i++) {
    pinMode(ALARM_PINS[i], OUTPUT);
    digitalWrite(ALARM_PINS[i], LOW); // Apaga los LEDs de alarma inicialmente
  }

  if(!USE_TEST_MODE){
    // Configuración de pines de switch como entrada con pull-up
    pinMode(MODE_SWITCH_PIN, INPUT_PULLUP); // Modo AC/DC
    pinMode(USART_SWITCH_PIN, INPUT_PULLUP); // Control USART

    // Inicializar comunicación serial
    Serial.begin(SERIAL_BAUD_RATE);
  }

  // Configuración de pines ADC como entrada (implícito en Arduino)
  for(int i = 0; i < NUM_CHANNELS; i++) {
    pinMode(ADC_PINS[i], INPUT);
  }

  // Mostrar voltajes iniciales en la pantalla
  displayManager.displayVoltages(realVoltages, alarms);
  displayManager.displayAlarms(alarms);
}

// -----------------------------
// Función loop
// -----------------------------

/*
  La función loop() se ejecuta continuamente.
  Aquí se leen los voltajes, se procesan, se manejan las alarmas, se actualiza la pantalla,
  y se envían los datos a la PC si está habilitado.
*/
void loop() {
  if(USE_TEST_MODE){
    // Modo de prueba: utilizar voltajes simulados
    static unsigned long previousMillis = 0;
    const long interval = 5000; // Intervalo de 5 segundos para cambiar los datos simulados

    unsigned long currentMillis = millis();

    // Verifica si ha pasado el intervalo de tiempo para simular cambios
    if(currentMillis - previousMillis >= interval){
      previousMillis = currentMillis; // Actualiza el tiempo anterior

      // Simula diferentes escenarios de voltajes y alarmas
      static int cycle = 0; // Variable para llevar el conteo de ciclos
      cycle++;              // Incrementa el ciclo
      switch(cycle % 4) {   // Cambia de escenario cada 4 ciclos
        case 0:
          // Escenario 1: Sin alarmas, todos en modo DC
          realVoltages[0] = 12.34;
          realVoltages[1] = 15.00;
          realVoltages[2] = 5.67;
          realVoltages[3] = 19.80;
          break;
        case 1:
          // Escenario 2: Alarma en CH2 y CH4, Modo AC en CH1 y CH4
          realVoltages[0] = 10.00;
          realVoltages[1] = -22.50;
          realVoltages[2] = 7.89;
          realVoltages[3] = 21.50;
          break;
        case 2:
          // Escenario 3: Múltiples alarmas, todos en modo DC
          realVoltages[0] = 25.00;
          realVoltages[1] = -25.00;
          realVoltages[2] = 30.00;
          realVoltages[3] = -30.00;
          break;
        case 3:
          // Escenario 4: Alarma en CH3, Modo AC en CH2
          realVoltages[0] = 18.75;
          realVoltages[1] = 20.00;
          realVoltages[2] = 22.00;
          realVoltages[3] = 19.00;
          break;
      }

      // Actualiza el modo AC/DC basado en el ciclo
      // Para simplificar, alternamos el modo globalmente en cada ciclo
      currentMode = (cycle % 2 == 0) ? false : true;

      // Actualiza los modos individuales si es necesario
      // Aquí, dependiendo del escenario, se pueden ajustar los modos por canal

      // Manejar alarmas y actualizar LEDs
      handleAlarms(realVoltages);

      // Actualizar la pantalla con los nuevos voltajes
      displayManager.displayVoltages(realVoltages, alarms);

      // Verifica si hay alguna alarma activa
      bool anyAlarm = false;
      for(int i = 0; i < NUM_CHANNELS; i++) {
        if(alarms[i]){
          anyAlarm = true; // Si hay al menos una alarma, marca la bandera
          break;           // No es necesario seguir revisando
        }
      }

      if(anyAlarm){
        // Si hay alarmas, las muestra en la pantalla
        displayManager.displayAlarms(alarms);
      } else {
        // Si no hay alarmas, limpia la sección de alarmas
        displayManager.clearAlarms();
      }
    }
  }
  // Para la entrega sacamos toda la lógica del else y eliminamos del todo el if
  else{
    // Modo Real: leer voltajes desde ADC
    unsigned long currentMillis = millis();

    // Leer el modo AC/DC desde el switch
    bool modeSwitch = readModeSwitch(); // True: AC, False: DC
    currentMode = modeSwitch;

    // Verificar si la transmisión serial está habilitada
    serialEnabled = isSerialEnabledFunc();

    // Leer voltajes de cada canal
    for(int i = 0; i < NUM_CHANNELS; i++) {
      int adcValue = analogRead(ADC_PINS[i]); // Leer el valor del ADC
      float scaledVoltage;

      if(currentMode) { // Modo AC
        // Almacenar muestras para cálculo RMS
        samples[sampleIndex][i] = scaleVoltage(adcValue);
      }
      else { // Modo DC
        // Escalar voltaje directamente
        scaledVoltage = scaleVoltage(adcValue);
        realVoltages[i] = scaledVoltage;
      }
    }

    if(currentMode){
      // Manejar muestras para AC
      if(currentMillis - lastSampleTime >= sampleInterval){
        lastSampleTime = currentMillis;
        sampleIndex++;
        if(sampleIndex >= NUM_SAMPLES){
          // Calcular RMS para cada canal
          for(int i = 0; i < NUM_CHANNELS; i++) {
            float rms = calculateRMS(samples[i], NUM_SAMPLES);
            realVoltages[i] = rms;
          }
          sampleIndex = 0;
        }
      }
    }

    // Manejar alarmas y actualizar LEDs
    handleAlarms(realVoltages);

    // Actualizar la pantalla con los nuevos voltajes
    displayManager.displayVoltages(realVoltages, alarms);

    // Verificar si hay alguna alarma activa
    bool anyAlarm = false;
    for(int i = 0; i < NUM_CHANNELS; i++) {
      if(alarms[i]){
        anyAlarm = true; // Si hay al menos una alarma, marca la bandera
        break;           // No es necesario seguir revisando
      }
    }

    if(anyAlarm){
      // Si hay alarmas, las muestra en la pantalla
      displayManager.displayAlarms(alarms);
    }
    else{
      // Si no hay alarmas, limpia la sección de alarmas
      displayManager.clearAlarms();
    }

    // Enviar datos vía serial si está habilitado
    if(serialEnabled){
      if(currentMillis - lastSerialSend >= serialInterval){
        lastSerialSend = currentMillis;

        // Formatear los datos en formato CSV
        String csvData = "";
        for(int i = 0; i < NUM_CHANNELS; i++) {
          csvData += String(realVoltages[i], 2);
          if(i < NUM_CHANNELS - 1){
            csvData += ",";
          }
        }
        csvData += "\n";

        // Enviar los datos por serial
        Serial.print(csvData);
      }
    }
  }
}
