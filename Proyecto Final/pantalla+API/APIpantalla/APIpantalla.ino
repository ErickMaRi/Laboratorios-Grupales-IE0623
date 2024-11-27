#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Credenciales WiFi
const char* ssid = "Pangolin";
const char* password = "Anchia.94";

// Configuración de Endpoint
const String serverUrl = "https://bq19t3sb3d.execute-api.us-east-2.amazonaws.com/Prod/get_calendar_data"; // Endpoint
const String serverUrlnew = "https://bq19t3sb3d.execute-api.us-east-2.amazonaws.com/Prod/check_and_create_meeting"; // Endpoint

#define TFT_CS     D3
#define TFT_DC     D2
#define TFT_RST    D4

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

#define MAX_NOMBRE_LEN 50
#define MAX_FECHA_LEN 11
#define MAX_HORA_LEN 6
#define MAX_DESCRIPCION_LEN 200

#define VENTANA_EVENTOS 5

#define BUTTON_PREV_PIN     D1
#define BUTTON_NEXT_PIN     D6

struct EventoData {
  char nombreEvento[MAX_NOMBRE_LEN];
  char fechaEvento[MAX_FECHA_LEN];
  char horaEvento[MAX_HORA_LEN];
  char descripcion[MAX_DESCRIPCION_LEN];
};

EventoData eventosData[VENTANA_EVENTOS];
int numEventosEnVentana = 0;
int indiceEventoEnVentana = 0;
int indiceEventoGlobal = 0;

bool masEventosDisponibles = true;
int totalEventosDisponibles = 0;

char csvData[2048];
char tempLine[256];

class Evento {
  public:
    char nombreEvento[MAX_NOMBRE_LEN];
    char fechaEvento[MAX_FECHA_LEN];
    char horaEvento[MAX_HORA_LEN];
    char descripcion[MAX_DESCRIPCION_LEN];
    int totalDescriptionLength;
    int scrollPixelIndex;

    static const int MAX_LINEAS_DESCRIPCION = 4;
    static const int CHARS_PER_LINE = 16;
    static const int PIXELS_PER_CHAR = 6;
    Evento() : scrollPixelIndex(0), totalDescriptionLength(0) {}

    void cargarEvento(EventoData& evento) {
      strncpy(nombreEvento, evento.nombreEvento, MAX_NOMBRE_LEN - 1);
      nombreEvento[MAX_NOMBRE_LEN - 1] = '\0';

      strncpy(fechaEvento, evento.fechaEvento, MAX_FECHA_LEN - 1);
      fechaEvento[MAX_FECHA_LEN - 1] = '\0';

      strncpy(horaEvento, evento.horaEvento, MAX_HORA_LEN - 1);
      horaEvento[MAX_HORA_LEN - 1] = '\0';

      strncpy(descripcion, evento.descripcion, MAX_DESCRIPCION_LEN - 1);
      descripcion[MAX_DESCRIPCION_LEN - 1] = '\0';

      totalDescriptionLength = strlen(descripcion);
      scrollPixelIndex = 0;
    }

    void dibujarEstaticos() {
      tft.fillScreen(ST7735_BLACK);

      int16_t cursorY = 0;
      tft.setTextWrap(false);

      tft.setTextColor(ST7735_MAGENTA);
      tft.setTextSize(2);

      int16_t x1, y1;
      uint16_t w, h;
      tft.getTextBounds(nombreEvento, 0, cursorY, &x1, &y1, &w, &h);
      int16_t posX = (tft.width() - w) / 2;
      tft.setCursor(posX, cursorY);
      tft.print(nombreEvento);
      cursorY += h + 5;

      tft.drawFastHLine(5, cursorY, tft.width() - 10, ST7735_WHITE);
      cursorY += 5;

      tft.setTextColor(ST7735_CYAN);
      tft.setTextSize(1);

      tft.setCursor(5, cursorY);
      tft.print("Fecha: ");
      tft.println(fechaEvento);
      cursorY += 10;

      tft.setCursor(5, cursorY);
      tft.print("Hora:  ");
      tft.println(horaEvento);
      cursorY += 10;

      tft.drawFastHLine(5, cursorY, tft.width() - 10, ST7735_WHITE);
      cursorY += 5;
    }

    void dibujarDescripcion() {
      int16_t descripcionY = 50;

      tft.setTextColor(ST7735_WHITE);
      tft.setTextSize(2);
      tft.setTextWrap(false);

      for (int line = 0; line < MAX_LINEAS_DESCRIPCION; line++) {
        int16_t yPos = descripcionY + (line * 18);
        tft.fillRect(0, yPos, tft.width(), 18, ST7735_BLACK);

        int16_t xOffset = -(scrollPixelIndex % PIXELS_PER_CHAR);

        tft.setCursor(5 + xOffset, yPos);

        int charStart = (scrollPixelIndex / PIXELS_PER_CHAR) + (line * CHARS_PER_LINE);
        if (charStart >= totalDescriptionLength) {
          charStart = totalDescriptionLength;
        }

        char lineBuffer[CHARS_PER_LINE + 1];
        for (int i = 0; i < CHARS_PER_LINE; i++) {
          if ((charStart + i) < totalDescriptionLength) {
            lineBuffer[i] = descripcion[charStart + i];
          } else {
            lineBuffer[i] = ' ';
          }
        }
        lineBuffer[CHARS_PER_LINE] = '\0';

        tft.print(lineBuffer);
      }
    }

    void actualizarScroll(int16_t scrollSpeed) {
      scrollPixelIndex += scrollSpeed;

      int totalPixels = totalDescriptionLength * PIXELS_PER_CHAR;

      if (scrollPixelIndex >= totalPixels) {
        scrollPixelIndex = 0;
      }

      dibujarDescripcion();
    }
};

Evento eventoActual;

bool lastButtonPrevState = HIGH;
bool lastButtonNextState = HIGH;

unsigned long buttonPrevPressTime = 0;
unsigned long buttonNextPressTime = 0;

const unsigned long HOLD_TIME_MS = 2000;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "time.google.com", -21600, 60000);

void cargarEventos(int indiceInicio, bool crearNuevo);
int determinarEventoActual();
void manejarBotones();
void testInternetConnectivity();
void anadirEvento();

void setup() {
  Serial.begin(115200);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);

  tft.fillScreen(ST7735_RED);
  delay(500);
  tft.fillScreen(ST7735_GREEN);
  delay(500);
  tft.fillScreen(ST7735_BLUE);
  delay(500);
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.println("Inicializando...");

  pinMode(BUTTON_PREV_PIN, INPUT_PULLUP);
  pinMode(BUTTON_NEXT_PIN, INPUT_PULLUP);

  WiFi.begin(ssid, password);
  Serial.println("Conectando a WiFi...");
  tft.println("Conectando a WiFi...");
  int wifiRetryCount = 0;
  while (WiFi.status() != WL_CONNECTED && wifiRetryCount < 20) {
    delay(1000);
    Serial.print(".");
    tft.print(".");
    wifiRetryCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado a WiFi");
    tft.println("\nWiFi Conectado");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    testInternetConnectivity();

    Serial.println("Iniciando sincronización NTP...");
    tft.println("Sincronizando tiempo...");
    timeClient.begin();

    int ntpRetryCount = 0;
    while (!timeClient.update() && ntpRetryCount < 10) {
      Serial.println("Intentando actualizar NTP...");
      timeClient.forceUpdate();
      ntpRetryCount++;
      delay(500);
    }

    if (ntpRetryCount >= 10) {
      Serial.println("No se pudo sincronizar el tiempo NTP.");
      tft.println("Error NTP");
    } else {
      Serial.println("Tiempo sincronizado:");
      Serial.println(timeClient.getFormattedTime());

      tft.fillScreen(ST7735_BLACK);
      tft.setCursor(0, 0);
      tft.setTextSize(2);
      tft.setTextColor(ST7735_GREEN);
      tft.println("Hora actual:");
      tft.println(timeClient.getFormattedTime());
      delay(2000);

      cargarEventos(indiceEventoGlobal, false);

      Serial.print("Total de eventos disponibles: ");
      Serial.println(totalEventosDisponibles);
      tft.fillScreen(ST7735_BLACK);
      tft.setCursor(0, 0);
      tft.setTextSize(2);
      tft.setTextColor(ST7735_YELLOW);
      tft.println("Eventos:");
      tft.println(totalEventosDisponibles);
      delay(2000);

      if (numEventosEnVentana > 0) {
        eventoActual.cargarEvento(eventosData[indiceEventoEnVentana]);
        eventoActual.dibujarEstaticos();
        eventoActual.dibujarDescripcion();
      } else {
        Serial.println("No hay eventos para mostrar.");
        tft.println("Sin eventos");
      }
    }
  } else {
    Serial.println("\nNo se pudo conectar a WiFi");
    tft.println("\nError WiFi");
    return;
  }
}

void loop() {
  static unsigned long lastScrollUpdate = 0;
  unsigned long currentTime = millis();
  const int16_t scrollSpeed = 1;
  const unsigned long scrollInterval = 20;

  manejarBotones();

  if (numEventosEnVentana > 0) {
    if (currentTime - lastScrollUpdate >= scrollInterval) {
      lastScrollUpdate = currentTime;
      eventoActual.actualizarScroll(scrollSpeed);
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    timeClient.update();
  }
}

void manejarBotones() {
  bool buttonPrevState = digitalRead(BUTTON_PREV_PIN);
  bool buttonNextState = digitalRead(BUTTON_NEXT_PIN);
  unsigned long currentTime = millis();

  // Botón Previo
  if (buttonPrevState == LOW && lastButtonPrevState == HIGH) {
    buttonPrevPressTime = currentTime;
  }
  if (buttonPrevState == HIGH && lastButtonPrevState == LOW) {
    unsigned long pressDuration = currentTime - buttonPrevPressTime;
    if (pressDuration >= HOLD_TIME_MS) {
      Serial.println("Botón Previo mantenido: Ir al evento actual");
      indiceEventoGlobal = determinarEventoActual();
      cargarEventos(indiceEventoGlobal, false);
      indiceEventoEnVentana = 0;
      eventoActual.cargarEvento(eventosData[indiceEventoEnVentana]);
      eventoActual.dibujarEstaticos();
      eventoActual.dibujarDescripcion();
    } else {
      Serial.println("Botón Previo presionado");
      if (indiceEventoGlobal > 0) {
        indiceEventoGlobal--;
        indiceEventoEnVentana--;
        if (indiceEventoEnVentana < 0) {
          int nuevoIndiceInicio = indiceEventoGlobal - VENTANA_EVENTOS + 1;
          if (nuevoIndiceInicio < 0) nuevoIndiceInicio = 0;
          cargarEventos(nuevoIndiceInicio, false);
          indiceEventoEnVentana = indiceEventoGlobal - nuevoIndiceInicio;
        }
        eventoActual.cargarEvento(eventosData[indiceEventoEnVentana]);
        eventoActual.dibujarEstaticos();
        eventoActual.dibujarDescripcion();
      } else {
        Serial.println("Primer evento.");
        tft.println("Primer evento");
      }
    }
  }
  lastButtonPrevState = buttonPrevState;

  // Botón Siguiente
  if (buttonNextState == LOW && lastButtonNextState == HIGH) {
    buttonNextPressTime = currentTime;
  }
  if (buttonNextState == HIGH && lastButtonNextState == LOW) {
    unsigned long pressDuration = currentTime - buttonNextPressTime;
    if (pressDuration >= HOLD_TIME_MS) {
      Serial.println("Botón Siguiente mantenido: Añadir evento");
      anadirEvento();
    } else {
      Serial.println("Botón Siguiente presionado");
      indiceEventoGlobal++;
      indiceEventoEnVentana++;
      if (indiceEventoEnVentana >= numEventosEnVentana) {
        cargarEventos(indiceEventoGlobal, false);
        indiceEventoEnVentana = 0;
      }
      eventoActual.cargarEvento(eventosData[indiceEventoEnVentana]);
      eventoActual.dibujarEstaticos();
      eventoActual.dibujarDescripcion();
    }
  }
  lastButtonNextState = buttonNextState;
}

void anadirEvento() {
  Serial.println("Botón Siguiente mantenido: Reservar espacio");
  indiceEventoGlobal = determinarEventoActual();
  cargarEventos(indiceEventoGlobal, true);
  indiceEventoEnVentana = 0;
  eventoActual.cargarEvento(eventosData[indiceEventoEnVentana]);
  eventoActual.dibujarEstaticos();
  eventoActual.dibujarDescripcion();
}

void cargarEventos(int indiceInicio, bool crearNuevo) {
  numEventosEnVentana = 0;
  indiceEventoEnVentana = 0;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;

  Serial.println("Solicitando datos...");
  tft.println("Solicitando datos...");
  if (crearNuevo){
  https.begin(client, serverUrlnew);
  }else{
  https.begin(client, serverUrl);
  }
  int httpResponseCode = https.GET();

  Serial.print("Código de respuesta HTTP: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode > 0) {
    String payload = https.getString();
    Serial.println("Respuesta del servidor:");
    Serial.println(payload);
    tft.println("Datos recibidos");

    int indexStart = payload.indexOf("\"file_content\":\"");
    if (indexStart == -1) {
      Serial.println("No se encontró 'file_content'.");
      tft.println("Sin datos");
      https.end();
      masEventosDisponibles = false;
      return;
    }

    indexStart += strlen("\"file_content\":\"");
    int indexEnd = payload.indexOf("\",\"", indexStart);
    if (indexEnd == -1) {
      indexEnd = payload.indexOf("\"}}", indexStart);
      if (indexEnd == -1) {
        Serial.println("No se pudo encontrar el final de 'file_content'.");
        tft.println("Error de datos");
        https.end();
        masEventosDisponibles = false;
        return;
      }
    }

    String fileContent = payload.substring(indexStart, indexEnd);
    fileContent.replace("\\n", "\n");
    fileContent.replace("\\\"", "\"");
    fileContent.replace("\\\\", "\\");

    int eventosCargados = 0;
    int eventoIndex = 0;
    totalEventosDisponibles = 0;
    char* line;
    char* ptr;

    memset(csvData, 0, sizeof(csvData));
    fileContent.toCharArray(csvData, sizeof(csvData) - 1);

    line = strtok_r(csvData, "\n", &ptr);
    Serial.println("Encabezado CSV:");
    Serial.println(line);

    line = strtok_r(NULL, "\n", &ptr);

    while (line != NULL) {
      totalEventosDisponibles++;
      line = strtok_r(NULL, "\n", &ptr);
    }

    memset(csvData, 0, sizeof(csvData));
    fileContent.toCharArray(csvData, sizeof(csvData) - 1);
    line = strtok_r(csvData, "\n", &ptr);
    line = strtok_r(NULL, "\n", &ptr);

    eventoIndex = 0;
    while (line != NULL && eventoIndex < indiceInicio) {
      line = strtok_r(NULL, "\n", &ptr);
      eventoIndex++;
    }

    while (line != NULL && eventosCargados < VENTANA_EVENTOS) {
      Serial.print("Procesando línea: ");
      Serial.println(line);

      memset(tempLine, 0, sizeof(tempLine));
      strncpy(tempLine, line, sizeof(tempLine) - 1);

      char* tokenPtr;
      char* startStr = strtok_r(tempLine, ",", &tokenPtr);
      char* endStr = strtok_r(NULL, ",", &tokenPtr);
      char* summaryStr = strtok_r(NULL, ",", &tokenPtr);
      char* descriptionStr = strtok_r(NULL, ",", &tokenPtr);
      char* locationStr = strtok_r(NULL, ",", &tokenPtr);
      char* attendeesStr = strtok_r(NULL, ",", &tokenPtr);

      if (startStr == NULL || summaryStr == NULL) {
        Serial.println("Datos incompletos en la línea CSV.");
        break;
      }

      char fechaEvento[MAX_FECHA_LEN];
      strncpy(fechaEvento, startStr, 10);
      fechaEvento[10] = '\0';

      char horaEvento[MAX_HORA_LEN];
      strncpy(horaEvento, startStr + 11, 5);
      horaEvento[5] = '\0';

      strncpy(eventosData[eventosCargados].nombreEvento, summaryStr, MAX_NOMBRE_LEN - 1);
      eventosData[eventosCargados].nombreEvento[MAX_NOMBRE_LEN - 1] = '\0';

      strncpy(eventosData[eventosCargados].fechaEvento, fechaEvento, MAX_FECHA_LEN - 1);
      eventosData[eventosCargados].fechaEvento[MAX_FECHA_LEN - 1] = '\0';

      strncpy(eventosData[eventosCargados].horaEvento, horaEvento, MAX_HORA_LEN - 1);
      eventosData[eventosCargados].horaEvento[MAX_HORA_LEN - 1] = '\0';

      if (descriptionStr != NULL) {
        strncpy(eventosData[eventosCargados].descripcion, descriptionStr, MAX_DESCRIPCION_LEN - 1);
        eventosData[eventosCargados].descripcion[MAX_DESCRIPCION_LEN - 1] = '\0';
      } else {
        eventosData[eventosCargados].descripcion[0] = '\0';
      }

      eventosCargados++;
      numEventosEnVentana = eventosCargados;

      line = strtok_r(NULL, "\n", &ptr);
    }

    if (eventosCargados < VENTANA_EVENTOS) {
      masEventosDisponibles = false;
    }

    https.end();

  } else {
    Serial.print("Error en la solicitud GET. Código: ");
    Serial.println(httpResponseCode);
    tft.print("Error GET: ");
    tft.println(httpResponseCode);
    https.end();
    masEventosDisponibles = false;
    return;
  }
}

int determinarEventoActual() {
  time_t rawTime = timeClient.getEpochTime();
  struct tm* timeInfo = localtime(&rawTime);

  char buffer[25];
  strftime(buffer, 25, "%Y-%m-%dT%H:%M", timeInfo);
  String tiempoActual = String(buffer);

  Serial.print("Tiempo actual: ");
  Serial.println(tiempoActual);

  String fechaActual = tiempoActual.substring(0, 10);
  String horaActual = tiempoActual.substring(11, 16);

  Serial.print("Fecha actual: ");
  Serial.println(fechaActual);
  Serial.print("Hora actual: ");
  Serial.println(horaActual);

  for (int i = 0; i < numEventosEnVentana; i++) {
    String eventoFecha = String(eventosData[i].fechaEvento);
    String eventoHora = String(eventosData[i].horaEvento);

    Serial.print("Comparando con evento ");
    Serial.println(i);
    Serial.print("Fecha del evento: ");
    Serial.println(eventoFecha);
    Serial.print("Hora del evento: ");
    Serial.println(eventoHora);

    if (eventoFecha.equals(fechaActual) && eventoHora.equals(horaActual)) {
      Serial.println("¡Evento actual encontrado!");
      return indiceEventoGlobal + i;
    }
  }

  Serial.println("No se encontró un evento actual.");
  return 0;
}

void testInternetConnectivity() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, "http://example.com");
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.println("Conectividad a Internet OK.");
    } else {
      Serial.println("Falló la conectividad a Internet.");
    }
    http.end();
  }
}
