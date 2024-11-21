import serial
import csv
from datetime import datetime
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import threading
import time
import sys

# El código produce un gráfico dinámico con la ventana de los tres ejes del acelerómetro

# Definimos el puerto
PORT = '/dev/ttyACM0'
BAUDRATE = 115200
TIMEOUT = 1

# Constantes
SAMPLE_RATE = 100  # Frecuencia de muestreo en Hz
BUFFER_SIZE = SAMPLE_RATE * 1  # Buffer para 3 segundos de datos

# Buffers para los datos
ax_data = deque([0.0]*BUFFER_SIZE, maxlen=BUFFER_SIZE)
ay_data = deque([0.0]*BUFFER_SIZE, maxlen=BUFFER_SIZE)
az_data = deque([0.0]*BUFFER_SIZE, maxlen=BUFFER_SIZE)

# Lock para manejar acceso concurrente al guardar datos
data_lock = threading.Lock()

# Se declaran los ejes
fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 8))
ax1.set_title('Aceleración en X')
ax2.set_title('Aceleración en Y')
ax3.set_title('Aceleración en Z')
for ax in (ax1, ax2, ax3):
    ax.set_ylim([-2, 2])
    ax.set_xlim([0, BUFFER_SIZE])
    ax.grid(True)

line1, = ax1.plot([], [], lw=2)
line2, = ax2.plot([], [], lw=2)
line3, = ax3.plot([], [], lw=2)

# Bandera para detener el hilo
running = True

# Función de inicialización para FuncAnimation
def init():
    line1.set_data([], [])
    line2.set_data([], [])
    line3.set_data([], [])
    return line1, line2, line3

# Función para actualizar los gráficos
def update_plot(frame):
    with data_lock:
        line1.set_data(range(len(ax_data)), list(ax_data))
        line2.set_data(range(len(ay_data)), list(ay_data))
        line3.set_data(range(len(az_data)), list(az_data))
    return line1, line2, line3

# Función para guardar datos en CSV
def save_to_csv():
    with data_lock:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"acceleration_data_{timestamp}.csv"
        try:
            with open(filename, mode='w', newline='') as file:
                writer = csv.writer(file)
                writer.writerow(["ax", "ay", "az"])
                for ax, ay, az in zip(ax_data, ay_data, az_data):
                    writer.writerow([ax, ay, az])
            print(f"\nDatos guardados en {filename}")
        except Exception as e:
            print(f"\nError al guardar CSV: {e}")

# Función para leer datos del serial, con ella llenamos el buffer.
def read_serial():
    global running, ser
    while running:
        try:
            if not ser.is_open:
                ser.open()
                print("Puerto serial abierto.")
                time.sleep(2)  # Espera a que el Arduino se reinicie
            line = ser.readline().decode('utf-8').strip()
            if line:
                # Aquí podrías manejar otros mensajes de depuración si lo deseas
                try:
                    ax, ay, az = map(float, line.split(","))
                    with data_lock:
                        ax_data.append(ax)
                        ay_data.append(ay)
                        az_data.append(az)
                except ValueError:
                    print(f"Línea inválida: {line}")
        except serial.SerialException as e:
            print(f"SerialException: {e}. Intentando reconectar...")
            try:
                ser.close()
            except:
                pass
            time.sleep(2)  # Espera antes de intentar reconectar
        except UnicodeDecodeError as e:
            print(f"UnicodeDecodeError: {e}")

# Función para escuchar la entrada del usuario, con enter guardamos datos nuevos.
def listen_for_input():
    while running:
        try:
            user_input = input()
            if user_input == "":
                print("Guardando datos...")
                save_to_csv()
        except EOFError:
            # Se ejecuta cuando se cierra la entrada estándar
            break
        except Exception as e:
            print(f"Error en la entrada: {e}")

# Función para establecer la conexión serial
def connect_serial():
    global ser
    ser = serial.Serial(PORT, BAUDRATE, timeout=TIMEOUT)
    time.sleep(2)  # Espera a que el Arduino se reinicie

# Inicializar la conexión serial
try:
    connect_serial()
    print(f"Conectado al puerto serial {PORT} a {BAUDRATE} BAUD.")
except serial.SerialException as e:
    print(f"No se pudo abrir el puerto serial {PORT}: {e}")
    sys.exit(1)

# Iniciar el hilo de lectura serial
serial_thread = threading.Thread(target=read_serial, daemon=True)
serial_thread.start()

# Iniciar el hilo de escucha de entrada del usuario
input_thread = threading.Thread(target=listen_for_input, daemon=True)
input_thread.start()

# Configurar la animación
ani = animation.FuncAnimation(
    fig, 
    update_plot, 
    init_func=init, 
    blit=True, 
    interval=1000/SAMPLE_RATE,
    save_count=BUFFER_SIZE  # Limita el almacenamiento en caché
)

plt.tight_layout()
plt.show()

# Detener los hilos al cerrar la ventana
running = False
serial_thread.join()
input_thread.join()
try:
    ser.close()
    print("Puerto serial cerrado.")
except:
    pass
