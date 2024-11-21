import serial
import csv
from datetime import datetime
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import threading
import time
import sys

# Definimos el puerto
PORT = '/dev/ttyACM0'  # Ajusta este valor según tu sistema (e.g., 'COM3' en Windows)
BAUDRATE = 115200
TIMEOUT = 1

# Constantes
SAMPLE_RATE = 100  # Frecuencia de muestreo en Hz
BUFFER_SIZE = SAMPLE_RATE * 5  # Buffer para 5 segundos de datos

# Buffers para los datos del acelerómetro y las neuronas
ax_data = deque([0.0]*BUFFER_SIZE, maxlen=BUFFER_SIZE)
ay_data = deque([0.0]*BUFFER_SIZE, maxlen=BUFFER_SIZE)
az_data = deque([0.0]*BUFFER_SIZE, maxlen=BUFFER_SIZE)

prob1_data = deque([0.0]*BUFFER_SIZE, maxlen=BUFFER_SIZE)
prob2_data = deque([0.0]*BUFFER_SIZE, maxlen=BUFFER_SIZE)
prob3_data = deque([0.0]*BUFFER_SIZE, maxlen=BUFFER_SIZE)

movement_labels = deque(['']*BUFFER_SIZE, maxlen=BUFFER_SIZE)
infer_time_data = deque([0.0]*BUFFER_SIZE, maxlen=BUFFER_SIZE)

# Lock para manejar acceso concurrente al guardar datos
data_lock = threading.Lock()

# Se declaran los ejes para los datos
fig, axs = plt.subplots(6, 1, figsize=(12, 18))

# Acelerómetro plots
axs[0].set_title('Aceleración en X')
axs[1].set_title('Aceleración en Y')
axs[2].set_title('Aceleración en Z')

for ax in axs[:3]:
    ax.set_ylim([-2, 2])  # Ajusta según el rango de tus acelerómetros
    ax.set_xlim([0, BUFFER_SIZE])
    ax.grid(True)
    ax.legend(['Ax'])

# Model outputs plots
axs[3].set_title('Probabilidad Neurona 1')
axs[4].set_title('Probabilidad Neurona 2')
axs[5].set_title('Probabilidad Neurona 3')

for ax in axs[3:]:
    ax.set_ylim([0, 1])  # Probabilidades entre 0 y 1
    ax.set_xlim([0, BUFFER_SIZE])
    ax.grid(True)
    ax.legend(['Prob'])

# Initialize lines
line_ax, = axs[0].plot([], [], lw=2, label='Ax')
line_ay, = axs[1].plot([], [], lw=2, label='Ay', color='orange')
line_az, = axs[2].plot([], [], lw=2, label='Az', color='green')

line_prob1, = axs[3].plot([], [], lw=2, label='Prob1')
line_prob2, = axs[4].plot([], [], lw=2, label='Prob2', color='orange')
line_prob3, = axs[5].plot([], [], lw=2, label='Prob3', color='green')

for ax in axs:
    ax.legend(loc='upper right')

# Bandera para detener el hilo
running = True

# Función de inicialización para FuncAnimation
def init():
    line_ax.set_data([], [])
    line_ay.set_data([], [])
    line_az.set_data([], [])
    line_prob1.set_data([], [])
    line_prob2.set_data([], [])
    line_prob3.set_data([], [])
    return line_ax, line_ay, line_az, line_prob1, line_prob2, line_prob3

# Función para actualizar los gráficos
def update_plot(frame):
    with data_lock:
        x = range(len(ax_data))
        line_ax.set_data(x, list(ax_data))
        line_ay.set_data(x, list(ay_data))
        line_az.set_data(x, list(az_data))
        line_prob1.set_data(x, list(prob1_data))
        line_prob2.set_data(x, list(prob2_data))
        line_prob3.set_data(x, list(prob3_data))
    return line_ax, line_ay, line_az, line_prob1, line_prob2, line_prob3

# Función para guardar datos en CSV
def save_to_csv():
    with data_lock:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"model_outputs_{timestamp}.csv"
        try:
            with open(filename, mode='w', newline='') as file:
                writer = csv.writer(file)
                writer.writerow(["ax", "ay", "az", "prob1", "prob2", "prob3", "movementLabel", "infer_time"])
                # Convertir deque a listas
                ax_list = list(ax_data)
                ay_list = list(ay_data)
                az_list = list(az_data)
                prob1_list = list(prob1_data)
                prob2_list = list(prob2_data)
                prob3_list = list(prob3_data)
                movement_list = list(movement_labels)
                infer_time_list = list(infer_time_data)
                # Asumir que todas las listas tienen la misma longitud
                for i in range(len(ax_list)):
                    writer.writerow([
                        ax_list[i],
                        ay_list[i],
                        az_list[i],
                        prob1_list[i],
                        prob2_list[i],
                        prob3_list[i],
                        movement_list[i],
                        infer_time_list[i]
                    ])
            print(f"\nDatos guardados en {filename}")
        except Exception as e:
            print(f"\nError al guardar CSV: {e}")

# Función para leer datos del serial y llenar el buffer
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
                # Esperamos líneas en formato "ax,ay,az,prob1,prob2,prob3,movimientoLabel,infer_time"
                parts = line.split(",")
                if len(parts) == 8:
                    try:
                        ax = float(parts[0])
                        ay = float(parts[1])
                        az = float(parts[2])
                        prob1 = float(parts[3])
                        prob2 = float(parts[4])
                        prob3 = float(parts[5])
                        movement_label = parts[6]
                        infer_time = float(parts[7])
                        with data_lock:
                            ax_data.append(ax)
                            ay_data.append(ay)
                            az_data.append(az)
                            prob1_data.append(prob1)
                            prob2_data.append(prob2)
                            prob3_data.append(prob3)
                            movement_labels.append(movement_label)
                            infer_time_data.append(infer_time)
                    except ValueError:
                        print(f"Línea inválida (error de conversión): {line}")
                else:
                    print(f"Línea inválida (número de campos incorrecto): {line}")
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
    blit=False,  # Cambiado a False para mayor compatibilidad
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

