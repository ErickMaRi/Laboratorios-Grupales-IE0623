import serial
import time
import json
import requests

# Configuración del puerto serial (actualiza el puerto según corresponda)
SERIAL_PORT = '/dev/ttyACM0'  
BAUD_RATE = 115200  

# Configuración de ThingsBoard
THINGSBOARD_HOST = 'https://thingsboard.cloud'  
ACCESS_TOKEN = 'elzMPhnOoIIf8rJ5u5ZA'  

# Inicializar conexión serial
try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Conectado al puerto serial {SERIAL_PORT}")
except serial.SerialException as e:
    print(f"Error al conectar al puerto serial: {e}")
    exit(1)

# URL de la API de ThingsBoard
url = f"{THINGSBOARD_HOST}/api/v1/{ACCESS_TOKEN}/telemetry"

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
                datos = [d.strip() for d in linea.split(',')]

                if len(datos) == 4 and all(d != '' for d in datos):
                    try:
                        telemetria = {
                            'gyro_x': float(datos[0]),
                            'gyro_y': float(datos[1]),
                            'gyro_z': float(datos[2]),
                            'bateria': (50)(float(datos[3])-7)
                        }
                        enviar_datos_thingsboard(telemetria)
                    except ValueError as e:
                        print(f"Error al convertir datos a float: {e}")
                else:
                    print("Formato de datos incorrecto o campos vacíos")
            time.sleep(0.1)
        except UnicodeDecodeError as e:
            print(f"Error de decodificación: {e}")
            continue
        except Exception as e:
            print(f"Error en la lectura del puerto serial: {e}")
            time.sleep(1)

if __name__ == '__main__':
    main()