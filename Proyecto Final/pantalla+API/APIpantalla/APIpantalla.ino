#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// Credenciales WiFi
const char* ssid = "Apartamento JM";         
const char* password = "JMQVLDFM.07"; 

// Configuración de ThingsBoard
const String accessToken = "3803hSuCGv298cVRIrgX"; // Token de acceso del dispositivo
const String serverUrl = "https://iot.eie.ucr.ac.cr/api/v1/" + accessToken + "/attributes"; // Endpoint

// Configuración de la pantalla TFT
#define TFT_CS     D3   
#define TFT_DC     D2   
#define TFT_RST    D4   

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Dimensiones de la pantalla
const int16_t SCREEN_WIDTH = 160;
const int16_t SCREEN_HEIGHT = 128;

// Definir constantes para los tamaños 
#define MAX_NOMBRE_LEN 20      
#define MAX_FECHA_LEN 11       // Formato "YYYY-MM-DD"
#define MAX_HORA_LEN 6         // Formato "HH:MM"
#define MAX_DESCRIPCION_LEN 150 

#define MAX_EVENTS 5 

// Estructura para almacenar los datos de eventos
struct EventoData {
  char nombreEvento[MAX_NOMBRE_LEN];
  char fechaEvento[MAX_FECHA_LEN];
  char horaEvento[MAX_HORA_LEN];
  char descripcion[MAX_DESCRIPCION_LEN];
};

EventoData eventosData[MAX_EVENTS];
int numEventos = 0; // Número actual de eventos almacenados

// Clase Evento contiene la carga y visualización de eventos
class Evento {
  public:
    char nombreEvento[MAX_NOMBRE_LEN];
    char fechaEvento[MAX_FECHA_LEN];
    char horaEvento[MAX_HORA_LEN];
    char descripcion[MAX_DESCRIPCION_LEN];
    int totalDescriptionLength; // Longitud total de la descripción en caracteres
    int scrollPixelIndex; // Índice de desplazamiento en píxeles

    static const int MAX_LINEAS_DESCRIPCION = 4; // Máximo número de líneas a mostrar
    static const int CHARS_PER_LINE = 16; // Número de caracteres por línea
    static const int PIXELS_PER_CHAR = 6; // Ancho de cada carácter en píxeles 
    Evento() : scrollPixelIndex(0), totalDescriptionLength(0) {}

    // Función para cargar un evento desde eventosData
    void cargarEvento(int index) {
      // Copia los datos del evento
      strncpy(nombreEvento, eventosData[index].nombreEvento, MAX_NOMBRE_LEN - 1);
      nombreEvento[MAX_NOMBRE_LEN - 1] = '\0';

      strncpy(fechaEvento, eventosData[index].fechaEvento, MAX_FECHA_LEN - 1);
      fechaEvento[MAX_FECHA_LEN - 1] = '\0';

      strncpy(horaEvento, eventosData[index].horaEvento, MAX_HORA_LEN - 1);
      horaEvento[MAX_HORA_LEN - 1] = '\0';

      strncpy(descripcion, eventosData[index].descripcion, MAX_DESCRIPCION_LEN - 1);
      descripcion[MAX_DESCRIPCION_LEN - 1] = '\0';

      // Calcula la longitud total de la descripción
      totalDescriptionLength = strlen(descripcion);

      // Reinicia el índice de desplazamiento
      scrollPixelIndex = 0;
    }

    // Método para dibujar los elementos estáticos (título, fecha y hora)
    void dibujarEstaticos() {
      // Limpia la pantalla
      tft.fillScreen(ST7735_BLACK);

      int16_t cursorY = 0;
      tft.setTextWrap(false);

      // Muestra el nombre del evento centrado
      tft.setTextColor(ST7735_MAGENTA);
      tft.setTextSize(2);

      int16_t x1, y1;
      uint16_t w, h;
      tft.getTextBounds(nombreEvento, 0, cursorY, &x1, &y1, &w, &h);
      int16_t posX = (SCREEN_WIDTH - w) / 2;
      tft.setCursor(posX, cursorY);
      tft.print(nombreEvento);
      cursorY += h + 5;

      // Dibuja una línea separadora
      tft.drawFastHLine(5, cursorY, SCREEN_WIDTH - 10, ST7735_WHITE);
      cursorY += 5;

      // Muestra detalles de fecha y hora
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

      // Dibuja otra línea separadora
      tft.drawFastHLine(5, cursorY, SCREEN_WIDTH - 10, ST7735_WHITE);
      cursorY += 5;
    }

    // Método para dibujar la descripción (dinámica)
    void dibujarDescripcion() {
      // Posición inicial para la descripción
      int16_t descripcionY = 50; // Ajustar según sea necesario

      // Configuramos el texto
      tft.setTextColor(ST7735_WHITE);
      tft.setTextSize(2);
      tft.setTextWrap(false);

      // Por cada línea
      for (int line = 0; line < MAX_LINEAS_DESCRIPCION; line++) {
        // Limpiamos la línea antes de dibujar
        int16_t yPos = descripcionY + (line * 18); // Ajustar el espaciado de líneas según sea necesario
        tft.fillRect(0, yPos, SCREEN_WIDTH, 18, ST7735_BLACK);

        // Calculamos el desplazamiento en píxeles para esta línea
        int16_t xOffset = -(scrollPixelIndex % PIXELS_PER_CHAR);

        // Establecemos el puntero con el desplazamiento aplicado
        tft.setCursor(5 + xOffset, yPos);

        // Determinamos el inicio del texto para esta línea
        int charStart = (scrollPixelIndex / PIXELS_PER_CHAR) + (line * CHARS_PER_LINE);
        charStart = charStart % totalDescriptionLength; // Envolver para evitar exceder la longitud

        // Se crea un buffer temporal para la línea
        char lineBuffer[CHARS_PER_LINE + 1];
        for (int i = 0; i < CHARS_PER_LINE; i++) {
          int charIndex = (charStart + i) % totalDescriptionLength;
          lineBuffer[i] = descripcion[charIndex];
        }
        lineBuffer[CHARS_PER_LINE] = '\0';

        // Finalmente dibujamos la línea
        tft.print(lineBuffer);
      }
    }

    // Para actualizar el desplazamiento
    void actualizarScroll(int16_t scrollSpeed) {
      // Se actualiza el índice respecto la velocidad
      scrollPixelIndex += scrollSpeed;

      // Calcula el total de pixeles
      int totalPixels = totalDescriptionLength * PIXELS_PER_CHAR;

      // Se enrolla el valor si supera el total
      if (scrollPixelIndex >= totalPixels) {
        scrollPixelIndex = 0;
      }

      // Dibujar solo la descripción con el nuevo desplazamiento
      dibujarDescripcion();
    }
};

// Instancia del evento actual
Evento eventoActual;
int indiceEvento = 0;

void setup() {
  Serial.begin(115200);

  // Inicializa la pantalla
  tft.initR(INITR_BLACKTAB);  // Inicializar la pantalla con la configuración Black Tab
  tft.setRotation(1);

  // Prueba de pantalla
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

  // Configuración de WiFi
  WiFi.begin(ssid, password);
  Serial.println("Conectando a WiFi...");
  tft.println("Conectando a WiFi...");
  int wifiRetryCount = 0;
  while (WiFi.status() != WL_CONNECTED && wifiRetryCount < 10) { // Intentar durante 10 segundos
    delay(1000);
    Serial.print(".");
    tft.print(".");
    wifiRetryCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado a WiFi");
    tft.println("\nWiFi Conectado");

    // Configuración del cliente HTTPS
    WiFiClientSecure client; // Cliente seguro para HTTPS
    client.setInsecure(); // Ignorar la verificación de certificados SSL

    HTTPClient https;

    Serial.println("Iniciando solicitud GET...");
    tft.println("Solicitando datos...");
    https.begin(client, serverUrl); // Configurar la URL del servidor con WiFiClientSecure para HTTPS
    int httpResponseCode = https.GET(); // Realiza la solicitud GET

    if (httpResponseCode > 0) {
      // Si la solicitud fue exitosa
      String payload = https.getString();
      Serial.println("Respuesta del servidor:");
      Serial.println(payload);
      tft.println("Datos recibidos");

      // Analizar la respuesta JSON
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print(F("deserializeJson() falló: "));
        Serial.println(error.f_str());
        tft.println("Error JSON");
        return;
      }

      // Extraer "file_content"
      const char* fileContent = doc["client"]["file_content"];
      if (fileContent == NULL) {
        Serial.println("No se encontró 'file_content' en la respuesta JSON.");
        tft.println("Sin datos");
        return;
      }

      // Procesar los datos CSV
      char csvData[512]; // Se ajusta a un tamaño más pequeño
      strncpy(csvData, fileContent, sizeof(csvData) - 1);
      csvData[sizeof(csvData) - 1] = '\0';

      char* line = strtok(csvData, "\n"); // Obtener la primera línea (encabezado)
      Serial.println("Encabezado CSV:");
      Serial.println(line);

      line = strtok(NULL, "\n"); // Obtener la primera línea de datos

      while (line != NULL && numEventos < MAX_EVENTS) {
        Serial.print("Procesando línea: ");
        Serial.println(line);
        // Analizar la línea
        // Dividir la línea por comas
        char* startStr = strtok(line, ",");
        char* endStr = strtok(NULL, ",");
        char* summaryStr = strtok(NULL, ",");
        char* descriptionStr = strtok(NULL, ",");
        char* locationStr = strtok(NULL, ",");
        char* attendeesStr = strtok(NULL, ",");

        if (startStr == NULL || summaryStr == NULL) {
          Serial.println("Datos incompletos en la línea CSV.");
          break;
        }

        // Extraer fecha y hora de startStr
        // Formato esperado: "YYYY-MM-DDTHH:MM:SS-06:00"
        // Copiar fecha
        char fechaEvento[MAX_FECHA_LEN];
        strncpy(fechaEvento, startStr, 10); // Copiar los primeros 10 caracteres, "YYYY-MM-DD"
        fechaEvento[10] = '\0';

        // Copiar hora
        char horaEvento[MAX_HORA_LEN];
        strncpy(horaEvento, startStr + 11, 5); // Copiar "HH:MM"
        horaEvento[5] = '\0';

        // Almacenar los campos en eventosData[numEventos]
        strncpy(eventosData[numEventos].nombreEvento, summaryStr, MAX_NOMBRE_LEN - 1);
        eventosData[numEventos].nombreEvento[MAX_NOMBRE_LEN - 1] = '\0';

        strncpy(eventosData[numEventos].fechaEvento, fechaEvento, MAX_FECHA_LEN - 1);
        eventosData[numEventos].fechaEvento[MAX_FECHA_LEN - 1] = '\0';

        strncpy(eventosData[numEventos].horaEvento, horaEvento, MAX_HORA_LEN - 1);
        eventosData[numEventos].horaEvento[MAX_HORA_LEN - 1] = '\0';

        if (descriptionStr != NULL) {
          strncpy(eventosData[numEventos].descripcion, descriptionStr, MAX_DESCRIPCION_LEN - 1);
          eventosData[numEventos].descripcion[MAX_DESCRIPCION_LEN - 1] = '\0';
        } else {
          eventosData[numEventos].descripcion[0] = '\0';
        }

        numEventos++;
        yield(); // Ceder tiempo al sistema operativo

        // Obtiene la siguiente línea
        line = strtok(NULL, "\n");
      }

    } else {
      // Si hay un error en la solicitud
      Serial.print("Error en la solicitud GET. Código: ");
      Serial.println(httpResponseCode);
      tft.print("Error GET: ");
      tft.println(httpResponseCode);
      return;
    }

    https.end(); // Cerrar la conexión

  } else {
    Serial.println("\nNo se pudo conectar a WiFi");
    tft.println("\nError WiFi");
    return;
  }

  if (numEventos > 0) {
    // Cargar y mostrar el primer evento si lo hay
    eventoActual.cargarEvento(indiceEvento);
    eventoActual.dibujarEstaticos();
    eventoActual.dibujarDescripcion();
  } else {
    Serial.println("No hay eventos para mostrar.");
    tft.println("Sin eventos");
  }
}



void loop() {
  static unsigned long lastScrollUpdate = 0;
  unsigned long currentTime = millis();
  const int16_t scrollSpeed = 1; // Velocidad de desplazamiento (píxeles por actualización)
  const unsigned long scrollInterval = 20; // Intervalo de actualización en ms para desplazamiento suave

  if (numEventos > 0) {
    // Actualizar el desplazamiento de la descripción
    if (currentTime - lastScrollUpdate >= scrollInterval) {
      lastScrollUpdate = currentTime;

      eventoActual.actualizarScroll(scrollSpeed);
    }

    // Cambiar de evento después de un cierto tiempo
    static unsigned long lastEventChange = 0;
    const unsigned long eventChangeInterval = 20000; // Cambiar de evento cada 20 segundos

    if (currentTime - lastEventChange >= eventChangeInterval) {
      lastEventChange = currentTime;
      indiceEvento = (indiceEvento + 1) % numEventos;

      // Cargar el nuevo evento
      eventoActual.cargarEvento(indiceEvento);
      eventoActual.dibujarEstaticos();
      eventoActual.dibujarDescripcion();
    }
  }
}
