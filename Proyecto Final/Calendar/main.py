from fastapi import FastAPI, Form, Request, HTTPException
from fastapi.responses import JSONResponse
import os.path
import mangum

from fastapi.responses import HTMLResponse
from fastapi.templating import Jinja2Templates
import datetime
import csv
import os
import json
from google.oauth2.credentials import Credentials
from google.auth.transport.requests import Request as GRequest
from googleapiclient.discovery import build
from googleapiclient.errors import HttpError
from io import StringIO

# Definir el alcance
SCOPES = ["https://www.googleapis.com/auth/calendar"]

# Archivo para almacenar los tokens de acceso y actualización del usuario (en formato JSON)
TOKEN_JSON_FILE = 'token.json'

app = FastAPI()

# Directorio donde se almacenan plantillas HTML
templates = Jinja2Templates(directory="templates")


# Función auxiliar para obtener credenciales válidas
def get_credentials():
    creds = None
    if os.path.exists(TOKEN_JSON_FILE):
        with open(TOKEN_JSON_FILE, 'r') as token:
            creds_dict = json.load(token)
            creds = Credentials.from_authorized_user_info(creds_dict, SCOPES)

    # Si no hay credenciales válidas, iniciar el flujo de OAuth2
    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            try:
                creds.refresh(GRequest())
            except Exception as e:
                print(f"Error al refrescar el token: {e}")
                return None
        else:
            print("Es necesario volver a autenticar.")
            return None

    return creds

def obtener_eventos_y_enviar():
    creds = None
    # Comprobar si ya tenemos credenciales válidas almacenadas en el archivo token.json
    if os.path.exists(TOKEN_JSON_FILE):
        with open(TOKEN_JSON_FILE, 'r') as token:
            creds_dict = json.load(token)
            # Asegurarse de que creds_dict tenga el formato correcto
            if isinstance(creds_dict, dict):
                creds = Credentials.from_authorized_user_info(creds_dict, SCOPES)
            else:
                print("Los datos del token no están en el formato esperado.")
                return None

    # Si no hay credenciales válidas, solicitar al usuario que inicie sesión
    if not creds or not creds.valid:
        if creds and creds.expired and creds.refresh_token:
            try:
                # Intentar refrescar el token usando el token de actualización
                request = GRequest()
                creds.refresh(request)
                print("Token refrescado correctamente.")
            except Exception as e:
                print(f"Error al refrescar el token: {e}")
                print("El token de actualización probablemente ha expirado o ha sido revocado. Por favor, vuelva a autenticarse.")
                return None
        else:
            print("No se puede refrescar el token, es necesario volver a autenticar.")
            return None

    # Imprimir el objeto de credenciales cargado para verificar que es correcto
    print(f"Credenciales cargadas: {creds}")
    try:
        service = build("calendar", "v3", credentials=creds)
        print("Servicio de Google Calendar construido correctamente.")

        # Comprobar si el servicio es válido
        if not service:
            print("No se pudo construir el servicio de Google Calendar.")
            return None

        # Establecer el rango de tiempo para las siguientes 24 horas
        ahora = datetime.datetime.now(datetime.timezone.utc).isoformat()
        tiempo_final = (datetime.datetime.now(datetime.timezone.utc) + datetime.timedelta(days=1)).isoformat()
        print("ahora", ahora)
        print("tiempo_final", tiempo_final)

        events_result = (
            service.events()
            .list(
                calendarId="primary",
                timeMin=ahora,  # Empezar desde el tiempo actual
                timeMax=tiempo_final,  # Terminar 24 horas después
                singleEvents=True,
                orderBy="startTime",
            )
            .execute()
        )
        eventos = events_result.get("items", [])

        if not eventos:
            print("No se encontraron reuniones para las siguientes 24 horas.")
            return None

        # Crear el contenido CSV
        csv_content = StringIO()
        writer = csv.writer(csv_content)
        writer.writerow(["Inicio", "Fin", "Resumen", "Descripción", "Ubicación", "Asistentes"])  # Fila de cabecera

        for evento in eventos:
            inicio = evento["start"].get("dateTime", evento["start"].get("date"))
            fin = evento["end"].get("dateTime", evento["end"].get("date"))

            resumen = evento.get("summary", "Sin título")
            descripcion = evento.get("description", "Sin descripción")
            ubicacion = evento.get("location", "Sin ubicación")
            asistentes = ", ".join(
                [asistente["email"] for asistente in evento.get("attendees", []) if "email" in asistente]
            ) or "Sin asistentes"

            writer.writerow([inicio, fin, resumen, descripcion, ubicacion, asistentes])

        return csv_content.getvalue()  # Devolver el contenido CSV como cadena

    except HttpError as error:
        print(f"Ocurrió un error: {error}")
        return None

# Endpoint de FastAPI para activar la recuperación de datos del Google Calendar y devolverlos como CSV
@app.get("/get_calendar_data")
def get_calendar_data():
    # Activar la función que recupera los datos del Google Calendar y los envía a ThingsBoard
    csv_content = obtener_eventos_y_enviar()

    if csv_content:
        # Devolver el contenido CSV envuelto en la estructura JSON requerida
        response_data = {"client": {"file_content": csv_content.replace('\r\n', '\n')}}
        return JSONResponse(content=response_data)
    else:
        return {"message": "No se encontraron eventos en el calendario para las próximas 24 horas."}

# Definir un formulario para crear una nueva reunión (esto se enviará a través de POST)
@app.get("/new_meeting", response_class=HTMLResponse)
async def new_meeting_form(request: Request):
    return templates.TemplateResponse("new_meeting_form.html", {"request": request})


# Endpoint para manejar el envío del formulario y crear un nuevo evento en el Google Calendar
@app.post("/create_meeting")
async def create_meeting(
    request: Request,
    start: str = Form(...),
    end: str = Form(...),
    summary: str = Form(...),
    description: str = Form(...),
    location: str = Form(...),
    attendees: str = Form(...),
):
    # Asegurarse de que las horas tengan la información de la zona horaria (por ejemplo, "-06:00")
    timezone = '-06:00'  # Ajustar según su zona horaria

    # Construir la hora de inicio y fin con la información de la zona horaria
    start_time = f'{start}:00{timezone}'
    end_time = f'{end}:00{timezone}'

    # Formatear los asistentes como una lista de diccionarios
    attendees_list = [{'email': attendee} for attendee in attendees.split(",")] if len(attendees) > 0 else []

    # Crear el diccionario de datos del evento
    event = {
        'summary': summary,
        'location': location,
        'description': description,
        'start': {
            'dateTime': start_time,
            'timeZone': timezone,
        },
        'end': {
            'dateTime': end_time,
            'timeZone': timezone,
        },
        'attendees': attendees_list,
    }

    print("Datos de la nueva reunión: ", event)
    # Obtener credenciales válidas y crear el evento en el Google Calendar
    creds = get_credentials()
    if not creds:
        raise HTTPException(status_code=401, detail="La autenticación del usuario falló.")

    try:
        service = build("calendar", "v3", credentials=creds)

        # Crear el evento en Google Calendar
        created_event = service.events().insert(calendarId="primary", body=event).execute()

        # Renderizar la página de éxito y pasar el ID del evento a la plantilla
        return templates.TemplateResponse("created.html", {
            "request": request,
            "event_id": created_event['id']
        })

    except HttpError as error:
        print(f"Ocurrió un error: {error}")
        return templates.TemplateResponse("failed.html", {
            "request": request,
            "error": str(error)
        })


@app.get("/check_and_create_meeting")
async def check_and_create_meeting(request: Request):
    # Obtener la hora actual y el bloque de tiempo para los próximos 30 minutos
    now = datetime.datetime.now(datetime.timezone.utc)
    start_time_check = now.isoformat()
    end_time_check = (now + datetime.timedelta(minutes=30)).isoformat()

    # Obtener credenciales válidas para interactuar con Google Calendar
    creds = get_credentials()
    if not creds:
        raise HTTPException(status_code=401, detail="La autenticación del usuario falló.")

    try:
        # Construir el servicio usando las credenciales
        service = build("calendar", "v3", credentials=creds)

        # Verificar si ya hay un evento en ese bloque de tiempo
        events_result = service.events().list(
            calendarId="primary",
            timeMin=start_time_check,
            timeMax=end_time_check,
            singleEvents=True,
            orderBy="startTime"
        ).execute()

        events = events_result.get("items", [])

        # Si no se encuentran eventos en el rango de tiempo, crear un nuevo evento
        if not events:
            # Preparar los datos para la nueva reunión
            timezone = '-06:00'  # Ajustar según su zona horaria
            start_time = now
            end_time = start_time + datetime.timedelta(minutes=30)  # Reunión de 30 hora

            # Crear los datos del evento
            event_data = {
                "summary": "Reunión creada desde el microcontrolador",
                "location": "Reunión Virtual",
                "description": "Una nueva reunión programada",
                "start": {
                    "dateTime": start_time.isoformat(),
                    "timeZone": timezone,
                },
                "end": {
                    "dateTime": end_time.isoformat(),
                    "timeZone": timezone,
                },
                "attendees": [],  # Agregar asistentes predeterminados si es necesario
            }

            # Insertar el nuevo evento en el Google Calendar
            created_event = service.events().insert(calendarId="primary", body=event_data).execute()

            print(f"Nuevo evento creado con ID: {created_event['id']}")

        # Ahora recuperar los eventos para las próximas 24 horas (incluyendo el nuevo si se crea)
        csv_content = obtener_eventos_y_enviar()  # Suponiendo que esta función devuelve los eventos en formato CSV

        if csv_content:
            # Devolver el contenido CSV envuelto en la estructura JSON requerida
            response_data = {"client": {"file_content": csv_content.replace('\r\n', '\n')}}
            return JSONResponse(content=response_data)
        else:
            return {"message": "No se encontraron eventos en el calendario para las próximas 24 horas."}

    except HttpError as error:
        print(f"Ocurrió un error: {error}")
        raise HTTPException(status_code=500, detail="No se pudo recuperar ni crear los eventos.")

# Adaptación para AWS Lambda
handler = mangum.Mangum(app)
