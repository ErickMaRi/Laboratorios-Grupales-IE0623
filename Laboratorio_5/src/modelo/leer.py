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
    carpetas = ['arriba', 'jalar', 'revés']
    inputs = []
    outputs = []

    # Read data from each folder and add to lists
    for i, carpeta in enumerate(carpetas):
        matriz_datos, identificador_archivos = leer_datos(carpeta, i)
        inputs.extend(matriz_datos)
        outputs.extend(identificador_archivos)

    num_max = max([max([max([abs(float(data)) for data in point])] for point in input) for input in inputs])[0]
    print("max accelaration:", num_max)
    for i in range(len(inputs)):
      for j in range(len(inputs[i])):
        for k in range(len(inputs[i][j])):
          inputs[i][j][k] = (float(inputs[i][j][k])+ num_max/2)/(2*num_max)

    inputs = [[data for point in input for data in point] for input in inputs]
    print(outputs)

    inputs = np.array(inputs, dtype=float)
    outputs = np.array(outputs, dtype=float)

    # Random shuffle
    indices = np.arange(len(inputs))
    np.random.shuffle(indices)
    inputs = inputs[indices]
    outputs = outputs[indices]

    # Data split: 60% train, 20% validation, 20% test
    num_entrenamiento = int(0.6 * len(inputs))
    num_validacion = int(0.2 * len(inputs))

    x_train, y_train = inputs[:num_entrenamiento], outputs[:num_entrenamiento]
    x_val, y_val = inputs[num_entrenamiento:num_entrenamiento + num_validacion], outputs[num_entrenamiento:num_entrenamiento + num_validacion]
    x_test, y_test = inputs[num_entrenamiento + num_validacion:], outputs[num_entrenamiento + num_validacion:]

    return x_train, y_train, x_val, y_val, x_test, y_test

def crear_modelo():
    modelo = tf.keras.models.Sequential([
        tf.keras.layers.Dense(128, activation='relu'),
        tf.keras.layers.Dropout(0.3),
        tf.keras.layers.Dense(64, activation='relu'),
        tf.keras.layers.Dropout(0.3),
        tf.keras.layers.Dense(3, activation='softmax')  # Three outputs for the three types of movement
    ])

    modelo.compile(optimizer='rmsprop', loss='mse', metrics=['mae'])
    return modelo
    
def main():
    # Prepare data
    input_train, output_train, input_val, output_val, input_test, output_test = preparar_datos()

    model = crear_modelo()
    model.fit(input_train, output_train, epochs=100, validation_data=(input_val, output_val))
    test_loss, test_acc = model.evaluate(input_test, output_test)
    print(f"Test loss: {test_loss}")
    print(f"Test accuracy: {test_acc}")


    # Obtener predicciones y calcular la matriz de confusión
    y_prediction = model.predict(input_test)
    y_prediction = np.argmax(y_prediction, axis=1)
    y_test = np.argmax(output_test, axis=1)

    # Crear matriz de confusión y normalizar por columnas
    conf_matrix = tf.math.confusion_matrix(y_test, y_prediction)
    conf_matrix = tf.cast(conf_matrix, dtype=tf.float32)
    conf_matrix_normalized = conf_matrix / tf.reduce_sum(conf_matrix, axis=0)

    # Imprimir la matriz de confusión normalizada
    print("Matriz de Confusión Normalizada:\n", conf_matrix_normalized)


    # Convert model to TensorFlow Lite
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_model = converter.convert()
    with open('/content/drive/MyDrive/modelo_movimiento.tflite', 'wb') as f:
        f.write(tflite_model)
    print("Model saved as 'modelo_movimiento.tflite' in Google Drive")
    print(input_test)

    # !echo "const unsigned char model[] = {" > /content/model.h
    # !cat /content/drive/MyDrive/modelo_movimiento.tflite | xxd -i      >> /content/model.h
    # !echo "};"                              >> /content/model.h

    # print("\nDone")

if __name__ == "__main__":
    main()