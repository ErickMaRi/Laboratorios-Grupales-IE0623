
//Definicion de variables globales
float v = 0.00;
float v_ac_pico = 0.00;
float v_ac = 0.00;

void setup(){
  //Configurar los pines como salida
  Serial.begin(9600);
  pinMode(2,OUTPUT);
  pinMode(A4, INPUT);
  pinMode(A0, INPUT);

}
void alarma(float v, int tipo){
  if (tipo == 0){ //AC
    if(v > 14.14) digitalWrite(2, HIGH); // encender LED
    else digitalWrite(2, LOW);
  }else if (tipo == 1){//DC
    if( v > 20 || v < -20) digitalWrite(2, HIGH);  // encender LED
    else digitalWrite(2, LOW);
  }
}

float calcular_rms() {
  v_ac_pico = 0.00;
  for(int i = 0; i < 100; i++) {
    v_ac = analogRead(A0); 
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


void loop(){
  Serial.print("Loop\n");
  if (analogRead(A4) < 512){ // Si switch cerrado medir AC
 
    //Leer y ajustar voltaje   
    v = calcular_rms();
    alarma(v, 0);

    //Pruebas
    Serial.print("Voltage AC: ");
    Serial.println(v);
    delay(1000);

  } 
  else{ // si switch abierto medir DC

    //Leer y ajustar voltaje
    v = analogRead(A0);
    v = 9.59743 * (v * 5 / 1023) -23.99999;
    alarma(v, 1);

    //Pruebas
    Serial.print("Voltage DC: ");
    Serial.println(v);
    delay(1000);
  }
}
