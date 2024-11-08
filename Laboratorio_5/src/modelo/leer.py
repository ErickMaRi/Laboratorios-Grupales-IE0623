import os
import csv

def leer_datos(carpeta, index):
    ruta_base = '../toma de datos/datos'  
    ruta_carpeta = os.path.join(ruta_base, carpeta)
    input = []  
    output= [] 

    # Verifica si la carpeta existe
    if not os.path.isdir(ruta_carpeta):
        print(f"La carpeta '{ruta_carpeta}' no existe.")
        return input, output

    # Recorrer cada archivo CSV en la carpeta
    for archivo in os.listdir(ruta_carpeta):
        if archivo.endswith('.csv'):
            ruta_archivo = os.path.join(ruta_carpeta, archivo)
            with open(ruta_archivo, 'r') as f:
                lector = csv.reader(f)
                datos_archivo = list(lector)[1:101]  # Omite el encabezado
                input.append(datos_archivo)
                # Añade el output correspondiente
                output.append([1 if i == index else 0 for i in range(3)]) # Si se usan m[as de 3 carpetas cambiar range

    return input, output

def main():
    # Carpetas a utilizar
    carpetas = ['arriba', 'jalar', 'girar muñeca clockwise'] 
    inputs = []
    outputs = []

    # Leer datos de cada carpeta y agregar a las listas
    for i, carpeta in enumerate(carpetas):
        matriz_datos, identificador_archivos = leer_datos(carpeta, i)
        inputs.extend(matriz_datos)
        outputs.extend(identificador_archivos)
        
    # for fila, outputs in zip(inputs, outputs):
    #     print(f"Inputs: {fila}")
    #     print(f"Outputs: {outputs}")
    #     print()

if __name__ == "__main__":
    main()
