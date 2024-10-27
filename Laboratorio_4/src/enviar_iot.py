import serial
import time
import json
import requests

# Configuración del puerto serial
ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)

# URL del servidor de ThingsBoard es un mock por ahora
THINGSBOARD_URL = 'http://demo.thingsboard.io/api/v1/NUESTRO_TOKEN_DE_ACCESO'

def enviar_datos(gyro, bateria):
    payload = {
        "gyro_x": gyro[0],
        "gyro_y": gyro[1],
        "gyro_z": gyro[2],
        "bateria": bateria
    }
    headers = {'Content-Type': 'application/json'}
    response = requests.post(THINGSBOARD_URL, data=json.dumps(payload), headers=headers)
    if response.status_code != 200:
        print("Error al enviar datos:", response.text)

while True:
    linea = ser.readline().decode('utf-8').strip()
    if linea:
        print("Recibido:", linea)
        try:
            datos = linea.split(',')
            gyro = [int(datos[0]), int(datos[1]), int(datos[2])]
            bateria = float(datos[3])
            enviar_datos(gyro, bateria)
        except Exception as e:
            print("Error procesando datos:", e)
    time.sleep(1)