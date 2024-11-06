import datetime
import os.path
import csv

from google.auth.transport.requests import Request
from google.oauth2.credentials import Credentials
from google_auth_oauthlib.flow import InstalledAppFlow
from googleapiclient.discovery import build
from googleapiclient.errors import HttpError

# Define the scope
SCOPES = ["https://www.googleapis.com/auth/calendar.readonly"]

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

    except HttpError as error:
        print(f"An error occurred: {error}")

if __name__ == "__main__":
    main()
