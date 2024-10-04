
//Definicion de variables globales
float v = 0.00;
float v_ac_pico = 0.00;
float v_ac = 0.00;

void setup(){
  //Configurar los pines como salida
  Serial.begin(9600);
  pinMode(2,OUTPUT);
  pinMode(3,OUTPUT);
  pinMode(4,OUTPUT);
  pinMode(5,OUTPUT);
  pinMode(A4, INPUT);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);

}
void alarma(float v, int tipo, int pin){
  //Encender Leds cuando se mida +-20V
  if (tipo == 0){ //AC
    if(v > 14.14) digitalWrite(pin, HIGH); // encender LED
    else digitalWrite(pin, LOW);
  }else if (tipo == 1){//DC
    if( v > 20 || v < -20) digitalWrite(pin, HIGH);  // encender LED
    else digitalWrite(pin, LOW);
  }
}

float calcular_rms(int pin) {
  v_ac_pico = 0.00;
  for(int i = 0; i < 100; i++) {
    v_ac = analogRead(pin); 
    v_ac = 9.59743 * (v_ac * 5 / 1023) -23.99999; 
    Serial.print("Voltage adjusted: ");
    Serial.println(v_ac);
    if(v_ac > v_ac_pico) v_ac_pico = v_ac;
    delayMicroseconds(100);
  }

    Serial.print("Voltage max: ");
    Serial.println(v_ac_pico);
    v_ac = v_ac_pico/sqrt(2); //Calcular RMS
    return v_ac;
}

float ajustar_dc(int pin){
  v = analogRead(pin);
  v = 9.59743 * (v * 5 / 1023) -23.99999;
}

void loop(){
  if (analogRead(A4) < 512){ // Si switch cerrado medir AC
 
    //Leer y ajustar voltaje   
    v = calcular_rms(A0);
    alarma(v, 0, 2);
    //Pruebas
    Serial.print("Voltage 1 AC : ");
    Serial.println(v);
    delay(1000);
    //Leer y ajustar voltaje   
    v = calcular_rms(A1);
    alarma(v, 0, 3);
    //Pruebas
    Serial.print("Voltage 1 AC : ");
    Serial.println(v);
    delay(1000);
    //Leer y ajustar voltaje   
    v = calcular_rms(A2);
    alarma(v, 0, 4);
    //Pruebas
    Serial.print("Voltage 1 AC : ");
    Serial.println(v);
    delay(1000);
    //Leer y ajustar voltaje   
    v = calcular_rms(A3);
    alarma(v, 0, 5);
    //Pruebas
    Serial.print("Voltage 1 AC : ");
    Serial.println(v);
    delay(1000);

  } 
  else{ // si switch abierto medir DC

    ajustar_dc(A0)
    alarma(v, 1, 2);
    //Pruebas
    Serial.print("Voltage 1 DC: ");
    Serial.println(v);
    delay(1000);
    ajustar_dc(A1)
    alarma(v, 1, 3);
    //Pruebas
    Serial.print("Voltage 2 DC: ");
    Serial.println(v);
    delay(1000);
    ajustar_dc(A2)
    alarma(v, 1, 4);
    //Pruebas
    Serial.print("Voltage  3 DC: ");
    Serial.println(v);
    delay(1000);
    ajustar_dc(A3)
    alarma(v, 1, 5);
    //Pruebas
    Serial.print("Voltage 4 DC: ");
    Serial.println(v);
    delay(1000);
  }
}
