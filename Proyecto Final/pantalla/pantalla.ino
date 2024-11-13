#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <avr/pgmspace.h>

// Definir pines de la pantalla TFT para SPI hardware
#define TFT_CS     9   // Pin de selección de chip (CS)
#define TFT_DC     10  // Pin de datos/comando (DC)
#define TFT_RST    8   // Pin de reinicio (RST)

// Crear una instancia de la pantalla usando SPI hardware
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Altura y ancho de la pantalla
const int16_t SCREEN_WIDTH = 160;
const int16_t SCREEN_HEIGHT = 128;

// Definir constantes para los tamaños máximos
#define MAX_NOMBRE_LEN 20   // Ajustado para ahorrar memoria
#define MAX_FECHA_LEN 11    // Formato "YYYY-MM-DD"
#define MAX_HORA_LEN 6      // Formato "HH:MM"
#define MAX_DESCRIPCION_LEN 150 // Ajustado para ahorrar memoria

// Estructura para almacenar los punteros a las cadenas en PROGMEM
struct EventoData {
  const char *nombreEvento;
  const char *fechaEvento;
  const char *horaEvento;
  const char *descripcion;
};

// Declaración de las cadenas en PROGMEM
const char nombreEvento0[] PROGMEM = "Reunión";
const char fechaEvento0[] PROGMEM = "2024-11-05";
const char horaEvento0[] PROGMEM = "14:00";
const char descripcion0[] PROGMEM = "_________-_-_-_-_---___________________-_-_-_-_---___________________-_-_-_-_---__________";

const char nombreEvento1[] PROGMEM = "Almuerzo";
const char fechaEvento1[] PROGMEM = "2024-11-05";
const char horaEvento1[] PROGMEM = "12:00";
const char descripcion1[] PROGMEM = "Almuerzo de equipo en el restaurante local. Oportunidad para fortalecer relaciones.";

const char nombreEvento2[] PROGMEM = "Conferencia";
const char fechaEvento2[] PROGMEM = "2024-11-06";
const char horaEvento2[] PROGMEM = "09:00";
const char descripcion2[] PROGMEM = "Conferencia anual de tecnología. Presentaciones de líderes de la industria.";

// Arreglo de estructuras EventoData en PROGMEM
const EventoData eventosData[] PROGMEM = {
  { nombreEvento0, fechaEvento0, horaEvento0, descripcion0 },
  { nombreEvento1, fechaEvento1, horaEvento1, descripcion1 },
  { nombreEvento2, fechaEvento2, horaEvento2, descripcion2 },
};

const int numEventos = sizeof(eventosData) / sizeof(eventosData[0]);

// Clase Evento que maneja la carga y visualización de eventos
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
    static const int PIXELS_PER_CHAR = 6; // Ancho de cada carácter en píxeles (ajustar según la fuente)

    Evento() : scrollPixelIndex(0), totalDescriptionLength(0) {}

    // Método para cargar un evento desde PROGMEM
    void cargarDesdePROGMEM(int index) {
      // Copiar la estructura EventoData desde PROGMEM
      EventoData eventoData;
      memcpy_P(&eventoData, &eventosData[index], sizeof(EventoData));

      // Copiar las cadenas desde PROGMEM a variables locales
      strcpy_P(nombreEvento, (PGM_P)eventoData.nombreEvento);
      strcpy_P(fechaEvento, (PGM_P)eventoData.fechaEvento);
      strcpy_P(horaEvento, (PGM_P)eventoData.horaEvento);
      strcpy_P(descripcion, (PGM_P)eventoData.descripcion);

      // Calcular la longitud total de la descripción
      totalDescriptionLength = strlen(descripcion);

      // Reiniciar el índice de desplazamiento
      scrollPixelIndex = 0;
    }

    // Método para dibujar los elementos estáticos (título, fecha y hora)
    void dibujarEstaticos() {
      // Limpiar la pantalla
      tft.fillScreen(ST7735_BLACK);

      int16_t cursorY = 0;
      tft.setTextWrap(false);

      // Mostrar el nombre del evento centrado
      tft.setTextColor(ST7735_MAGENTA);
      tft.setTextSize(2);

      int16_t x1, y1;
      uint16_t w, h;
      tft.getTextBounds(nombreEvento, 0, cursorY, &x1, &y1, &w, &h);
      int16_t posX = (SCREEN_WIDTH - w) / 2;
      tft.setCursor(posX, cursorY);
      tft.print(nombreEvento);
      cursorY += h + 5;

      // Dibujar una línea separadora
      tft.drawFastHLine(5, cursorY, SCREEN_WIDTH - 10, ST7735_WHITE);
      cursorY += 5;

      // Mostrar detalles de fecha y hora
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

      // Dibujar otra línea separadora
      tft.drawFastHLine(5, cursorY, SCREEN_WIDTH - 10, ST7735_WHITE);
      cursorY += 5;
    }

    // Método para dibujar la descripción con desplazamiento pixel por pixel
    void dibujarDescripcion() {
      // Posición inicial para la descripción
      int16_t descripcionY = 50; // Ajustar según sea necesario

      // Configuración del texto
      tft.setTextColor(ST7735_WHITE);
      tft.setTextSize(2);
      tft.setTextWrap(false);

      // Dibujar cada línea de la descripción
      for (int line = 0; line < MAX_LINEAS_DESCRIPCION; line++) {
        // Limpiar la línea antes de dibujar
        int16_t yPos = descripcionY + (line * 18); // Ajustar el espaciado de líneas según sea necesario
        tft.fillRect(0, yPos, SCREEN_WIDTH, 18, ST7735_BLACK);

        // Calcular el desplazamiento en píxeles para esta línea
        int16_t xOffset = -(scrollPixelIndex % PIXELS_PER_CHAR);

        // Establecer el cursor con el desplazamiento aplicado
        tft.setCursor(5 + xOffset, yPos);

        // Determinar el inicio del texto para esta línea
        int charStart = (scrollPixelIndex / PIXELS_PER_CHAR) + (line * CHARS_PER_LINE);
        charStart = charStart % totalDescriptionLength; // Envolver para evitar exceder la longitud

        // Crear un buffer temporal para la línea
        char lineBuffer[CHARS_PER_LINE + 1];
        for (int i = 0; i < CHARS_PER_LINE; i++) {
          int charIndex = (charStart + i) % totalDescriptionLength;
          lineBuffer[i] = descripcion[charIndex];
        }
        lineBuffer[CHARS_PER_LINE] = '\0';

        // Dibujar el texto de la línea
        tft.print(lineBuffer);
      }
    }

    // Método para actualizar el desplazamiento
    void actualizarScroll(int16_t scrollSpeed) {
      // Incrementar el índice de desplazamiento basado en la velocidad
      scrollPixelIndex += scrollSpeed;

      // Calcular el total de píxeles para todo el texto
      int totalPixels = totalDescriptionLength * PIXELS_PER_CHAR;

      // Envolver el índice de desplazamiento para mantenerlo dentro del rango
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
  // Inicializar la pantalla
  tft.initR(INITR_BLACKTAB);  // Inicializar la pantalla con la configuración Black Tab
  tft.setRotation(1); // Ajustar la rotación si es necesario

  // Cargar y mostrar el primer evento
  eventoActual.cargarDesdePROGMEM(indiceEvento);
  eventoActual.dibujarEstaticos();
  eventoActual.dibujarDescripcion();
}

void loop() {
  static unsigned long lastScrollUpdate = 0;
  unsigned long currentTime = millis();
  const int16_t scrollSpeed = 1; // Velocidad de desplazamiento (píxeles por actualización)
  const unsigned long scrollInterval = 20; // Intervalo de actualización en ms para desplazamiento suave

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
    eventoActual.cargarDesdePROGMEM(indiceEvento);
    eventoActual.dibujarEstaticos();
    eventoActual.dibujarDescripcion();
  }
}
