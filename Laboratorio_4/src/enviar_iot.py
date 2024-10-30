# sismografo_iot.py

import serial
import time
import json
import requests

# Configuración del puerto serial (actualiza el puerto según corresponda)
# SERIAL_PORT = 'COM3'  # En Windows, puede ser COM3, COM4, etc.
SERIAL_PORT = '/dev/ttyACM0'  # En Linux
BAUD_RATE = 115200  # Asegúrate de que coincida con la configuración del microcontrolador

# Configuración de ThingsBoard
THINGSBOARD_HOST = 'demo.thingsboard.io'
ACCESS_TOKEN = 'elzMPhnOoIIf8rJ5u5ZA'  # Reemplaza con el token de acceso del dispositivo

# Inicializar conexión serial
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Conectado al puerto serial {SERIAL_PORT}")
except serial.SerialException as e:
    print(f"Error al conectar al puerto serial: {e}")
    exit(1)

# URL de la API de ThingsBoard
url = f"http://{THINGSBOARD_HOST}/api/v1/{ACCESS_TOKEN}/telemetry"

def enviar_datos_thingsboard(telemetria):
    headers = {'Content-Type': 'application/json'}
    try:
        response = requests.post(url, data=json.dumps(telemetria), headers=headers)
        if response.status_code == 200:
            print("Datos enviados correctamente a ThingsBoard")
        else:
            print(f"Error al enviar datos: {response.status_code} {response.text}")
    except Exception as e:
        print(f"Excepción al enviar datos: {e}")

def main():
    while True:
        try:
            linea = ser.readline().decode('utf-8').strip()
            if linea:
                print(f"Datos recibidos: {linea}")
                # Suponiendo que los datos llegan en formato CSV: gyro_x,gyro_y,gyro_z,bateria
                datos = linea.split(',')
                if len(datos) == 4:
                    telemetria = {
                        'gyro_x': float(datos[0]),
                        'gyro_y': float(datos[1]),
                        'gyro_z': float(datos[2]),
                        'bateria': float(datos[3])
                    }
                    enviar_datos_thingsboard(telemetria)
                else:
                    print("Formato de datos incorrecto")
            time.sleep(0.1)
        except Exception as e:
            print(f"Error en la lectura del puerto serial: {e}")
            time.sleep(1)

if __name__ == '__main__':
    main()
