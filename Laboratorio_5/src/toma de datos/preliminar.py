import os
import glob
import pandas as pd
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np

# Guardamos en la carpeta datos
BASE_PATH = './datos'

# Recorremos las carpetas contenidas en datos para determinar las clases
clases = [d for d in os.listdir(BASE_PATH) if os.path.isdir(os.path.join(BASE_PATH, d))]

# Creamos los colores
colors = plt.cm.get_cmap('tab10', len(clases))

# Instanciamos el diccionario vacío
datos_por_clase = {clase: [] for clase in clases}

# Y por cada ítem de cada clase...
for idx, clase in enumerate(clases):
    carpeta_clase = os.path.join(BASE_PATH, clase)

    # Buscamos todos los .csv
    archivos_csv = glob.glob(os.path.join(carpeta_clase, '*.csv'))
    
    for archivo in archivos_csv:
        try:
            # Vamos intentando abrir cada archivo
            df = pd.read_csv(archivo)
            if {'ax', 'ay', 'az'}.issubset(df.columns):
                datos_por_clase[clase].append(df[['ax', 'ay', 'az']].values) # Leemos las entradas
            else:
                print(f"Advertencia: {archivo} no contiene las columnas 'ax', 'ay', 'az'.")
        except Exception as e:
            print(f"Error al leer {archivo}: {e}")

# Declaramos el plot por gesto
def plot_clase(clase, datos, color):
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    for muestra in datos:
        ax.plot(muestra[:,0], muestra[:,1], muestra[:,2], color=color, alpha=0.6)
    ax.set_title(f"Clase: {clase}")
    ax.set_xlabel('Ax')
    ax.set_ylabel('Ay')
    ax.set_zlabel('Az')
    plt.show()

# Hacemos plot de cada clase.
for idx, clase in enumerate(clases):
    plot_clase(clase, datos_por_clase[clase], colors(idx))

# 
fig = plt.figure()
ax = fig.add_subplot(111, projection='3d')

for idx, clase in enumerate(clases):
    color = colors(idx)
    for muestra in datos_por_clase[clase]:
        if muestra is datos_por_clase[clase][0]:
            etiqueta = clase
        else:
            etiqueta = ""
        ax.plot(muestra[:,0], muestra[:,1], muestra[:,2], color=color, alpha=0.6, label=etiqueta)
        
ax.set_title("Todas las Clases")
ax.set_xlabel('Ax')
ax.set_ylabel('Ay')
ax.set_zlabel('Az')

# Imprimimos todos los gestos agregados.
handles, labels = ax.get_legend_handles_labels()
unique = {}
for h, l in zip(handles, labels):
    if l not in unique:
        unique[l] = h
ax.legend(unique.values(), unique.keys())

plt.show()
