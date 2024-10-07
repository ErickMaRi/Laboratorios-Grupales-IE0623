import serial
import csv

# Configurar el puerto serial
serial_port = serial.Serial('/tmp/ttyS1', 9600)  # Ajustar según el puerto que se este usando

# Crear o abrir el archivo CSV
with open('voltimetro.csv', mode='w', newline='') as file:
    writer = csv.writer(file)
    writer.writerow(['V1', 'V2', 'V3', 'V4'])  # Escribir encabezado de las columnas
    
    try:
        while True:
            # Leer los datos del puerto serial
            data = serial_port.readline().decode('utf-8').strip()  # Decodificar y eliminar espacios en blanco
            
            # Dividir los datos en base al formato esperado (en este caso, por comas)
            voltages = data.split(',') 
            
            if len(voltages) == 4:  # Comprobar que hay cuatro valores
                writer.writerow(voltages)  # Escribir los datos de voltaje en el csv
            else:
                print(f"Datos no válidos: {data}")  # Si no hay cuatro voltajes, ignorar la línea
            
    except KeyboardInterrupt:
        print("Programa detenido")
    finally:
        serial_port.close()  # Cerrar el puerto serial al terminar
