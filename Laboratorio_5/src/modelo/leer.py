import os
import csv
import tensorflow as tf
import numpy as np

def leer_datos(carpeta, index):
    # Set base path to Google Drive folder
    ruta_base = '/content/drive/MyDrive/lab5micros/datos'  
    ruta_carpeta = os.path.join(ruta_base, carpeta)
    input = []  
    output= [] 

    # Check if folder exists
    if not os.path.isdir(ruta_carpeta):
        print(f"La carpeta '{ruta_carpeta}' no existe.")
        return input, output

    # Iterate over each CSV file in the folder
    for archivo in os.listdir(ruta_carpeta):
        if archivo.endswith('.csv'):
            ruta_archivo = os.path.join(ruta_carpeta, archivo)
            with open(ruta_archivo, 'r') as f:
                lector = csv.reader(f)
                datos_archivo = list(lector)[1:101]  # Skip header
                input.append(datos_archivo)
                # Append corresponding output
                output.append([1 if i == index else 0 for i in range(3)])  # Adjust range if using more than 3 folders

    return input, output

def preparar_datos():
    # Folders to use
    SEED = 42
    np.random.seed(SEED)
    tf.random.set_seed(SEED)
    carpetas = ['arriba', 'jalar', 'girar muñeca clockwise']
    inputs = []
    outputs = []

    # Read data from each folder and add to lists
    for i, carpeta in enumerate(carpetas):
        matriz_datos, identificador_archivos = leer_datos(carpeta, i)
        inputs.extend(matriz_datos)
        outputs.extend(identificador_archivos)

    inputs = np.array(inputs)
    outputs = np.array(outputs)
    
    # Random shuffle
    indices = np.arange(len(inputs))
    np.random.shuffle(indices)
    inputs = inputs[indices]
    outputs = outputs[indices]

    # Data split: 60% train, 20% validation, 20% test
    num_entrenamiento = int(0.6 * len(inputs))
    num_validacion = int(0.2 * len(inputs))

    x_train, y_train = inputs[:num_entrenamiento], outputs[:num_entrenamiento]
    x_val, y_val = inputs[num_entrenamiento:num_entrenamiento + num_validacion], outputs[num_entrenamiento:num_validacion + num_validacion]
    x_test, y_test = inputs[num_entrenamiento + num_validacion:], outputs[num_entrenamiento + num_validacion:]

    return x_train, y_train, x_val, y_val, x_test, y_test

def main():
    # Preparar datos
    x_train, y_train, x_val, y_val, x_test, y_test = preparar_datos()



if __name__ == "__main__":
    main()
