// ------------------------------------------------------------
// Juego "Simón dice" con ATtiny4313
// ------------------------------------------------------------

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <stdbool.h>

// Definición de pines para LEDs y botones
#define LED1 PB0
#define LED2 PB1
#define LED3 PB2
#define LED4 PB3

#define BOTON1 PD0
#define BOTON2 PD1
#define BOTON3 PD2
#define BOTON4 PD3

#define ZUMBADOR PB4 // Pin para el zumbador (OC1B)

// Estados del juego
typedef enum {
    ESPERAR_INICIO,
    INICIAR_SECUENCIA,
    MOSTRAR_SECUENCIA,
    ENTRADA_USUARIO,
    VERIFICAR_ENTRADA,
    JUEGO_TERMINADO
} EstadoJuego;

// Constantes y variables del juego
#define NIVEL_MAXIMO 10
volatile uint8_t secuencia[NIVEL_MAXIMO];
volatile uint8_t secuenciaUsuario[NIVEL_MAXIMO];
volatile uint8_t nivel = 0;
volatile uint8_t indiceEntrada = 0;
volatile uint8_t longitudSecuencia = 4; // Se inicia con 4 LEDs
volatile uint16_t tiempoEncendidoLED = 2000; // Tiempo inicial de LED encendido en ms
volatile uint8_t indiceSecuenciaActual = 0;
volatile EstadoJuego estadoActual = ESPERAR_INICIO;
volatile uint8_t botonPresionado = 0xFF; // 0xFF indica que no hay botón presionado
volatile uint16_t contadorISR = 0;

// Variables para temporizadores
volatile bool banderaTemporizador = false;

// Variable para el patrón de espera
volatile uint8_t indicePatronEspera = 0;

// Notas asociadas a cada LED/Botón (frecuencias en Hz)
#define NOTA1 262 // Do4
#define NOTA2 330 // Mi4
#define NOTA3 392 // Sol4
#define NOTA4 523 // Do5

// Prototipos de funciones
void configurar();
void inicializarTemporizadores();
void generarSecuencia();
void iniciarJuego();
void encenderLED(uint8_t led);
void reproducirSonido(uint8_t nota);
void apagarLEDs();
void apagarZumbador();
void juegoTerminado();
bool entradaCorrecta();
void incrementarDificultad();

int main(void) {
    configurar();       // Configura pines e interrupciones
    generarSecuencia(); // Genera la secuencia aleatoria

    while (1) {
        switch (estadoActual) {
            case ESPERAR_INICIO:
                // El patrón de LEDs se maneja en el ISR del Timer0
                if (botonPresionado != 0xFF) {
                    estadoActual = INICIAR_SECUENCIA;
                    botonPresionado = 0xFF; // Reiniciar el estado del botón
                }
                break;

            case INICIAR_SECUENCIA:
                iniciarJuego();  // Tonada corta y patrón inicial
                estadoActual = MOSTRAR_SECUENCIA;
                break;

            case MOSTRAR_SECUENCIA:
                // Mostrar secuencia
                if (banderaTemporizador) {
                    banderaTemporizador = false;
                    if (indiceSecuenciaActual < longitudSecuencia) {
                        encenderLED(secuencia[indiceSecuenciaActual]);
                        reproducirSonido(secuencia[indiceSecuenciaActual]);
                        // Esperar a que el LED se apague en la interrupción del timer
                    } else {
                        // Fin de la secuencia
                        indiceSecuenciaActual = 0;
                        estadoActual = ENTRADA_USUARIO;
                        apagarLEDs();
                        apagarZumbador();
                    }
                }
                break;

            case ENTRADA_USUARIO:
                if (botonPresionado != 0xFF) {
                    secuenciaUsuario[indiceEntrada++] = botonPresionado;
                    encenderLED(botonPresionado);
                    reproducirSonido(botonPresionado);

                    // Implementar antirrebote
                    _delay_ms(200);
                    apagarLEDs();
                    apagarZumbador();

                    botonPresionado = 0xFF; // Reiniciar el estado del botón

                    if (indiceEntrada >= longitudSecuencia) {
                        estadoActual = VERIFICAR_ENTRADA;
                    }
                }
                break;

            case VERIFICAR_ENTRADA:
                if (entradaCorrecta()) {
                    incrementarDificultad();
                    estadoActual = MOSTRAR_SECUENCIA;
                } else {
                    estadoActual = JUEGO_TERMINADO;
                }
                break;

            case JUEGO_TERMINADO:
                juegoTerminado();
                estadoActual = ESPERAR_INICIO;
                break;
        }
    }
    return 0;
}

/**
 * Configura los pines de entrada/salida, interrupciones y temporizadores.
 */
void configurar() {
    // Deshabilitar el USART
    UCSRB = 0;

    // Configurar LEDs como salidas
    DDRB |= (1 << LED1) | (1 << LED2) | (1 << LED3) | (1 << LED4);

    // Configurar zumbador como salida
    DDRB |= (1 << ZUMBADOR);

    // Apagar LEDs y zumbador inicialmente
    PORTB &= ~((1 << LED1) | (1 << LED2) | (1 << LED3) | (1 << LED4) | (1 << ZUMBADOR));

    // Configurar botones como entradas con pull-up activado
    DDRD &= ~((1 << BOTON1) | (1 << BOTON2) | (1 << BOTON3) | (1 << BOTON4));
    PORTD |= (1 << BOTON1) | (1 << BOTON2) | (1 << BOTON3) | (1 << BOTON4);

    // Configurar interrupciones por cambio de pin para PD0 - PD3
    PCMSK1 |= (1 << PCINT8) | (1 << PCINT9) | (1 << PCINT10) | (1 << PCINT11);
    GIMSK |= (1 << PCIE1); // Habilitar interrupciones por cambio de pin para PCINT[14:8]

    // Habilitar interrupción global
    sei();

    // Inicializar temporizadores
    inicializarTemporizadores();

    // Semilla para el generador de números aleatorios
    srand(1); // Puedes reemplazar 1 por una lectura de entrada aleatoria si es posible
}

/**
 * Inicializa los temporizadores para el control de LEDs y generación de sonido.
 */
void inicializarTemporizadores() {
    // Configurar Timer0 para control de tiempo de LEDs (Modo CTC)
    TCCR0A = (1 << WGM01); // Modo CTC

    // Con F_CPU = 1 MHz, Prescaler = 1024, queremos una interrupción cada 10 ms
    // Calcular OCR0A:
    // OCR0A = (F_CPU / (Prescaler * F_interrupt)) - 1
    // F_interrupt = 100 Hz (para 10 ms)
    // OCR0A = (1,000,000 / (1024 * 100)) - 1 = 9.76 - 1 ≈ 9

    TCCR0B = (1 << CS02) | (1 << CS00); // Prescaler de 1024
    OCR0A = 9; // Valor calculado para F_CPU = 1 MHz
    TIMSK |= (1 << OCIE0A); // Habilitar interrupción por comparación

    // Configurar Timer1 para generar frecuencia en el zumbador (OC1B - PB4)
    // Vamos a usar el modo CTC con prescaler de 8
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS11); // Modo CTC, prescaler 8
    OCR1A = 0; // Se establecerá en reproducirSonido()
}

/**
 * Genera una secuencia aleatoria de LEDs para el juego.
 */
void generarSecuencia() {
    for (uint8_t i = 0; i < NIVEL_MAXIMO; i++) {
        secuencia[i] = rand() % 4; // Números entre 0 y 3
    }
}

/**
 * Inicia el juego reproduciendo una tonada y parpadeando los LEDs.
 */
void iniciarJuego() {
    // Reproducir una tonada corta de inicio
    for (uint8_t i = 0; i < 4; i++) {
        reproducirSonido(i);
        _delay_ms(200);
        apagarZumbador();
        _delay_ms(100);
    }

    // Parpadear todos los LEDs 2 veces para indicar el inicio
    for (uint8_t i = 0; i < 2; i++) {
        PORTB |= (1 << LED1) | (1 << LED2) | (1 << LED3) | (1 << LED4);
        _delay_ms(500);
        apagarLEDs();
        _delay_ms(500);
    }

    // Reiniciar variables del juego
    nivel = 0;
    indiceEntrada = 0;
    longitudSecuencia = 4;
    tiempoEncendidoLED = 2000;
    indiceSecuenciaActual = 0;
}

/**
 * Enciende el LED especificado.
 *
 * @param led Número del LED a encender (0-3).
 */
void encenderLED(uint8_t led) {
    apagarLEDs(); // Asegurar que solo un LED esté encendido
    switch (led) {
        case 0:
            PORTB |= (1 << LED1);
            break;
        case 1:
            PORTB |= (1 << LED2);
            break;
        case 2:
            PORTB |= (1 << LED3);
            break;
        case 3:
            PORTB |= (1 << LED4);
            break;
    }
}

/**
 * Reproduce el sonido asociado a una nota.
 *
 * @param nota Número de la nota a reproducir (0-3).
 */
void reproducirSonido(uint8_t nota) {
    uint16_t frecuencia = 0;
    switch (nota) {
        case 0:
            frecuencia = NOTA1;
            break;
        case 1:
            frecuencia = NOTA2;
            break;
        case 2:
            frecuencia = NOTA3;
            break;
        case 3:
            frecuencia = NOTA4;
            break;
    }
    if (frecuencia == 0) {
        apagarZumbador();
        return;
    }
    // Calcular OCR1A para generar la frecuencia deseada
    // OCR1A = (F_CPU / (2 * Prescaler * Frecuencia)) - 1
    uint16_t valorOCR = (F_CPU / (2UL * 8UL * frecuencia)) - 1;
    OCR1A = valorOCR;
    TCCR1A |= (1 << COM1B0); // Activar toggle en OC1B
}

/**
 * Apaga todos los LEDs.
 */
void apagarLEDs() {
    PORTB &= ~((1 << LED1) | (1 << LED2) | (1 << LED3) | (1 << LED4));
}

/**
 * Apaga el zumbador.
 */
void apagarZumbador() {
    TCCR1A &= ~(1 << COM1B0); // Desactivar toggle en OC1B
    PORTB &= ~(1 << ZUMBADOR); // Asegurar que el zumbador esté apagado
}

/**
 * Maneja la secuencia cuando el juego termina.
 */
void juegoTerminado() {
    // Parpadear todos los LEDs 3 veces
    for (uint8_t i = 0; i < 3; i++) {
        PORTB |= (1 << LED1) | (1 << LED2) | (1 << LED3) | (1 << LED4);
        // Reproducir notas de aguda a grave
        for (int8_t j = 3; j >= 0; j--) {
            reproducirSonido(j);
            _delay_ms(100);
            apagarZumbador();
            _delay_ms(50);
        }
        _delay_ms(200);
        apagarLEDs();
        _delay_ms(200);
    }
}

/**
 * Verifica si la entrada del usuario es correcta.
 *
 * @return true si la entrada es correcta, false de lo contrario.
 */
bool entradaCorrecta() {
    for (uint8_t i = 0; i < longitudSecuencia; i++) {
        if (secuenciaUsuario[i] != secuencia[i]) {
            return false;
        }
    }
    return true;
}

/**
 * Incrementa la dificultad del juego aumentando la longitud de la secuencia y reduciendo el tiempo de encendido de los LEDs.
 */
void incrementarDificultad() {
    nivel++;
    longitudSecuencia++;
    if (longitudSecuencia > NIVEL_MAXIMO) {
        longitudSecuencia = NIVEL_MAXIMO;
    }
    if (tiempoEncendidoLED > 200) {
        tiempoEncendidoLED -= 200; // Reducir el tiempo de encendido en 200ms
    }
    indiceEntrada = 0;
    indiceSecuenciaActual = 0;
}

/**
 * Interrupción por cambio de pin para detectar botones presionados.
 */
ISR(PCINT1_vect) {
    contadorISR++;
    // Leer el estado de los botones y actualizar botonPresionado
    if (!(PIND & (1 << BOTON1))) {
        botonPresionado = 0;
    } else if (!(PIND & (1 << BOTON2))) {
        botonPresionado = 1;
    } else if (!(PIND & (1 << BOTON3))) {
        botonPresionado = 2;
    } else if (!(PIND & (1 << BOTON4))) {
        botonPresionado = 3;
    }
}

/**
 * Interrupción del Timer0 para controlar el tiempo y el patrón de espera.
 */
ISR(TIMER0_COMPA_vect) {
    static uint16_t contadorTemporizador = 0;
    contadorTemporizador += 10; // Incrementar cada 10 ms

    if (estadoActual == ESPERAR_INICIO) {
        static uint16_t temporizadorPatronEspera = 0;
        temporizadorPatronEspera += 10;

        if (temporizadorPatronEspera >= 500) { // Cambiar patrón cada 500 ms
            temporizadorPatronEspera = 0;
            apagarLEDs();
            encenderLED(indicePatronEspera);
            indicePatronEspera = (indicePatronEspera + 1) % 4;
        }
    } else if (estadoActual == MOSTRAR_SECUENCIA) {
        if (contadorTemporizador >= tiempoEncendidoLED) {
            contadorTemporizador = 0;
            apagarLEDs();
            apagarZumbador();
            indiceSecuenciaActual++;
            banderaTemporizador = true;
        }
    } else if (estadoActual == ENTRADA_USUARIO) {
        if (contadorTemporizador >= tiempoEncendidoLED) {
            contadorTemporizador = 0;
            apagarLEDs();
            apagarZumbador();
        }
    } else {
        // Reiniciar contador para otros estados
        contadorTemporizador = 0;
    }
}
