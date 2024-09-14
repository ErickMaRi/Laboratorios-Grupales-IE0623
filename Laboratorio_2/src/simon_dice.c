#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define ESPERA 0 //Modo de espera al inicio del juego.
#define INICIO 1 //Todos los LEDs parpadean 2 veces.
#define MOSTRAR 2 //Se muestra la secuencia, en este estado se realizan los ajustes al tiempo
#define REVISAR 3 //Se revisa que el usuario presione los colores correctos.
#define FIN 4 //Todos los LEDs parpadean 3 veces.

void parpadear(int n);
void delay(int ms);
void FSM();
void encenderLed(int n);
void imprimirSecuencia();
void iniciarsSecuencia();

volatile int overflow_cont;
int enable = 0;
int secuencia[13];
int seed = 137;
int turno = 1;
int estado = INICIO;

int main(void)
{
  //Variables
  overflow_cont = 0;
  
  // Timers internos
  TCNT0 = 0x00; // Inicializar Timer0. 
  TCCR0A = 0x00; // Normal 
  TCCR0B = 0b011; //Prescaling 64
  TIMSK = (1 << TOIE0); // Habilitar interrupción por desbordamiento de Timer0

  //Asignar interrupciones y puertos
  DDRB = 0x0F; // Configurar PB0, PB1, PB2 y PB3 como salidas
  GIMSK = 0xD8; //Habilitar interrupciones 
  PCMSK1 = 0b00000010; //PA1 puede disparar la interrupción
  PCMSK2 = 0b00000010; //PD1 puede disparar la interrupción
  MCUCR = 0x0A;

  sei();

  while(1){
    FSM();
  }
  //Usar para probar parpadear, borrar en version final
  // while (1) {
  //   parpadear(2);
  //   PORTB = 0x00;  // Apagar PB0, PB1, PB2 y PB3
  //   delay(5000); // Esperar 5s
  //   parpadear(3);
  //   PORTB = 0x00;  // Apagar PB0, PB1, PB2 y PB3
  //   delay(5000); // Esperar 5s
    
  // }

  // Usar para testear encenderLed, borrar luego
  // encenderLed(0);
  // delay(1000);
  // encenderLed(1);
  // delay(1000);
  // encenderLed(2);
  // delay(1000);
  // encenderLed(3);
  // delay(1000);
}   

// PORTB = 0x01; enciende 0
// PORTB = 0x02; enciende 1
// PORTB = 0x04; enciende 2
// PORTB = 0x08; enciende 3

void encenderLed(int n){
   switch(n){
    case 0:
      PORTB = 0x01;
      break;
    case 1:
      PORTB = 0x02;
      break;
    case 2:
      PORTB = 0x04;
      break;
    case 3:
      PORTB = 0x08;
      break;
    default:
      break;
   }
}

void parpadear(int n){
//Recibe el número de parpadeos a realizar
    for (int i = 0; i < n; i++) {
        PORTB = 0x0F;  // Encender PB0, PB1, PB2 y PB3
        delay(500); // Esperar 1s
        PORTB = 0x00;  // Apagar PB0, PB1, PB2 y PB3
        delay(500); // Esperar 1s
    }
}

void delay(int ms) {
    
    enable = 1; // Activar el contador de overflows
    overflow_cont = 0; // Reiniciar el contador
    TCNT0 = 0x00; // Reiniciar el contador del Timer0
    
    while (enable) {
        // Nada que hacer aquí, solo esperar a que el temporizador se desborde
        if(overflow_cont == ms){
          enable = 0; // Desactivar el contador de overflows
        }
    }
}

ISR(TIMER0_OVF_vect)
{ 
  if (enable){
    overflow_cont++;
  }
}

void imprimirSecuencia(){
    for (int i = 0; i < 3+turno; i++) {
        encenderLed(secuencia[i]);
        delay(2200-200*turno);
        PORTB = 0x00;
        delay(200);
    }
    PORTB = 0x00;
}

void iniciarsSecuencia() {
    // semilla
    srand(seed);
    
    for (int i = 0; i < 13; i++) {
        secuencia[i] = rand() % 4;  // valores de 0 to 3
    }
}

void FSM(){
  switch(estado){
    case ESPERA:
      // if (bpresiona algun boton{
      //   state = INICIO;       
      // }
      break;

    case INICIO:
      parpadear(2);
      estado = MOSTRAR;
      iniciarsSecuencia();      // generar el array de la secuencia de leds a encender
      break;

    case MOSTRAR:
      // encender los leds segun la secuencia y el turno
      imprimirSecuencia();
      estado = REVISAR;

      break;

    case REVISAR:
      // turno++;
      // estado = MOSTRAR;
      // if(turno == 14){
      //   estado = INICIO;
      // }
      break;
      //comparar el input con la secuencia
      //si pierde state = FIN; 
      //si lo hace bien: state = MOSTRAR; turno++
  
    case FIN:
      parpadear(3); // Parpadear LEDs 3 veces. 
      estado = INICIO;
      break;

    default:
      break;
  }
}