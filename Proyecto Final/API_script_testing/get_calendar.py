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

# Define the scope
SCOPES = ["https://www.googleapis.com/auth/calendar.readonly"]
# ThingsBoard configuration
THINGSBOARD_HOST = 'https://iot.eie.ucr.ac.cr/'
ACCESS_TOKEN = '06hyrjcbtwq2n5lrxzsv'
url = f"{THINGSBOARD_HOST}/api/v1/{ACCESS_TOKEN}/telemetry"
headers = {'Content-Type': 'application/json'}

def enviar_datos_thingsboard(telemetria):
    try:
        response = requests.post(url, data=json.dumps(telemetria), headers=headers)
        if response.status_code == 200:
            print("Data sent successfully to ThingsBoard")
        else:
            print(f"Error sending data: {response.status_code} {response.text}")
    except Exception as e:
        print(f"Exception sending data: {e}")

def send_calendar_data_to_thingsboard(csv_file_path):
    # Read the CSV file and send data to ThingsBoard
    with open(csv_file_path, newline='') as csvfile:
        reader = csv.DictReader(csvfile)
        for row in reader:
            telemetry_data = {
                "start": row["Start"],
                "end": row["End"],
                "summary": row["Summary"],
                "description": row["Description"],
                "location": row["Location"],
                "attendees": row["Attendees"]
            }
            enviar_datos_thingsboard(telemetry_data)

def main():
    """Retrieve meetings from Google Calendar starting now and for the next 24 hours, saving them in a CSV file."""
    creds = None
    # Check for existing credentials
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

        # Set time range for the next 24 hours
        now = datetime.datetime.now(datetime.timezone.utc).isoformat()
        end_time = (datetime.datetime.now(datetime.timezone.utc) + datetime.timedelta(days=1)).isoformat()

        print("Getting meetings for the next 24 hours")
        events_result = (
            service.events()
            .list(
                calendarId="primary",
                timeMin=now,       # Start from the current time
                timeMax=end_time,  # End 24 hours from now
                singleEvents=True,
                orderBy="startTime",
            )
            .execute()
        )
        events = events_result.get("items", [])

        if not events:
            print("No meetings found for the next 24 hours.")
            return

        # Write the events to a CSV file
        with open("next_24_hours_meetings.csv", "w", newline="") as file:
            writer = csv.writer(file)
            writer.writerow(["Start", "End", "Summary", "Description", "Location", "Attendees"])  # Header row

            # Filter out events that have already ended
            for event in events:
                start = event["start"].get("dateTime", event["start"].get("date"))
                end = event["end"].get("dateTime", event["end"].get("date"))

                # Skip events that have already ended
                if end <= now:
                    continue

                summary = event.get("summary", "No Title")
                description = event.get("description", "No Description")
                location = event.get("location", "No Location")
                attendees = ", ".join(
                    [attendee["email"] for attendee in event.get("attendees", []) if "email" in attendee]
                ) or "No Attendees"

                writer.writerow([start, end, summary, description, location, attendees])

        print("next_24_hours_meetings.csv created")
        csv_file_path = "next_24_hours_meetings.csv" 
        send_calendar_data_to_thingsboard(csv_file_path)

    except HttpError as error:
        print(f"An error occurred: {error}")

if __name__ == "__main__":
    main()