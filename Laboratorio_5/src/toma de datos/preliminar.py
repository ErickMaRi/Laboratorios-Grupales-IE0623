# -*- coding: utf-8 -*-
import os
import glob
import pandas as pd
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D  # Opcional en versiones recientes
import numpy as np

# Definir la ruta base donde se encuentran las carpetas de datos
BASE_PATH = './datos'

# Obtener la lista de clases (subcarpetas dentro de 'datos')
clases = [d for d in os.listdir(BASE_PATH) if os.path.isdir(os.path.join(BASE_PATH, d))]

# Asignar un color distinto a cada clase
colors = plt.cm.get_cmap('tab10', len(clases))

# Crear un diccionario para almacenar los datos por clase
datos_por_clase = {clase: [] for clase in clases}

# Recorrer cada clase y leer sus archivos CSV
for idx, clase in enumerate(clases):
    carpeta_clase = os.path.join(BASE_PATH, clase)
    # Encontrar todos los archivos CSV en la carpeta actual
    archivos_csv = glob.glob(os.path.join(carpeta_clase, '*.csv'))
    
    for archivo in archivos_csv:
        try:
            # Leer el CSV, asumiendo que tiene columnas 'ax', 'ay', 'az'
            df = pd.read_csv(archivo)
            if {'ax', 'ay', 'az'}.issubset(df.columns):
                datos_por_clase[clase].append(df[['ax', 'ay', 'az']].values)
            else:
                print(f"Advertencia: {archivo} no contiene las columnas 'ax', 'ay', 'az'.")
        except Exception as e:
            print(f"Error al leer {archivo}: {e}")

# Función para plotear una clase individual
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

# Plotear cada clase individualmente
for idx, clase in enumerate(clases):
    plot_clase(clase, datos_por_clase[clase], colors(idx))

# Plot combinado con todas las clases
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

# Crear una leyenda sin duplicados
handles, labels = ax.get_legend_handles_labels()
unique = {}
for h, l in zip(handles, labels):
    if l not in unique:
        unique[l] = h
ax.legend(unique.values(), unique.keys())

plt.show()
