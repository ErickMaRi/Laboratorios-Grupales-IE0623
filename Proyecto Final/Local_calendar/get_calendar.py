import datetime
import csv
import os.path
import requests
import json

from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build
from googleapiclient.errors import HttpError

# Definir el alcance
SCOPES = ["https://www.googleapis.com/auth/calendar"]
# Configuración de ThingsBoard
THINGSBOARD_HOST = 'https://iot.eie.ucr.ac.cr/'
ACCESS_TOKEN = '3803hSuCGv298cVRIrgX'
url = f"{THINGSBOARD_HOST}/api/v1/{ACCESS_TOKEN}/attributes"
headers = {'Content-Type': 'application/json'}


def leer_contenido_csv(ruta_archivo_csv):
    """Leer todo el contenido del archivo CSV como una cadena de texto."""
    with open(ruta_archivo_csv, "r") as archivo:
        return archivo.read()


def enviar_datos_a_thingsboard(atributos):
    try:
        response = requests.post(url, data=json.dumps(atributos), headers=headers)
        if response.status_code == 200:
            print("Datos enviados correctamente a ThingsBoard (atributos)")
        else:
            print(f"Error al enviar los datos: {response.status_code} {response.text}")
    except Exception as e:
        print(f"Excepción al enviar los datos: {e}")


def enviar_datos_del_calendario_a_thingsboard(ruta_archivo_csv):
    # Leer el contenido del archivo CSV
    contenido_csv = leer_contenido_csv(ruta_archivo_csv)
    atributos_datos = {
        "file_content": contenido_csv  # Solo almacenar el contenido del CSV
    }
    enviar_datos_a_thingsboard(atributos_datos)


def main():
    """Obtener las reuniones del Google Calendar comenzando desde ahora y para las siguientes 24 horas, guardándolas en un archivo CSV."""
    creds = None
    # Comprobar si existen credenciales guardadas
    if os.path.exists("token.json"):
        creds = Credentials.from_authorized_user_file("token.json", SCOPES)
    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            creds.refresh(Request())
        else:
            flow = InstalledAppFlow.from_client_secrets_file("credentials.json", SCOPES)
            creds = flow.run_local_server(port=0)
        with open("token.json", "w") as token:
            token.write(creds.to_json())

    try:
        service = build("calendar", "v3", credentials=creds)
        print("Google Calendar service successfully built.")

        # Check if the service is valid
        if not service:
            print("Failed to build the Google Calendar service.")
            return None

        calendar_list = service.calendarList().list().execute()
        print("Successfully fetched calendar list:")
        for calendar in calendar_list.get('items', []):
            print(f"Calendar: {calendar['summary']}")
        calendar_metadata = service.calendars().get(calendarId="primary").execute()


        print(f"Successfully retrieved calendar metadata: {calendar_metadata}")

        # Establecer el rango de tiempo para las siguientes 24 horas
        ahora = datetime.datetime.now(datetime.timezone.utc).isoformat()
        tiempo_final = (datetime.datetime.now(datetime.timezone.utc) + datetime.timedelta(days=1)).isoformat()

        print("Obteniendo reuniones para las siguientes 24 horas")
        events_result = (
            service.events()
            .list(
                calendarId="primary",
                timeMin=ahora,       # Empezar desde el tiempo actual
                timeMax=tiempo_final,  # Terminar 24 horas después
                singleEvents=True,
                orderBy="startTime",
            )
            .execute()
        )
        eventos = events_result.get("items", [])

        if not eventos:
            print("No se encontraron reuniones para las siguientes 24 horas.")
            return

        # Escribir los eventos en un archivo CSV
        with open("next_24_hours_meetings.csv", "w", newline="") as archivo:
            escritor = csv.writer(archivo)
            escritor.writerow(["Inicio", "Fin", "Resumen", "Descripción", "Ubicación", "Asistentes"])  # Fila de cabecera

            # Filtrar los eventos que ya han terminado
            for evento in eventos:
                inicio = evento["start"].get("dateTime", evento["start"].get("date"))
                fin = evento["end"].get("dateTime", evento["end"].get("date"))

                # Saltar eventos que ya han terminado
                if fin <= ahora:
                    continue

                resumen = evento.get("summary", "Sin título")
                descripcion = evento.get("description", "Sin descripción")
                ubicacion = evento.get("location", "Sin ubicación")
                asistentes = ", ".join(
                    [asistente["email"] for asistente in evento.get("attendees", []) if "email" in asistente]
                ) or "Sin asistentes"

                escritor.writerow([inicio, fin, resumen, descripcion, ubicacion, asistentes])

        print("Archivo next_24_hours_meetings.csv creado")
        ruta_archivo_csv = "next_24_hours_meetings.csv"
        enviar_datos_del_calendario_a_thingsboard(ruta_archivo_csv)


    except HttpError as error:
        print(f"Ocurrió un error: {error}")


if __name__ == "__main__":
    main()
