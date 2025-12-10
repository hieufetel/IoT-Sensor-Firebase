import serial
import time
import firebase_admin
from firebase_admin import credentials
from firebase_admin import firestore

# --- IMPORT YOUR CUSTOM PARSER ---
# This imports the file data_parser.py that handles the string logic
import data_parser

# --- 1. SETUP FIRESTORE (Your existing code) ---
cred = credentials.Certificate("serviceAccountKey.json")
firebase_admin.initialize_app(cred)
db = firestore.client()
print("Status: Firestore Connected.")

# --- 2. SETUP SERIAL (Hardware Connection) ---
# IMPORTANT: Check Device Manager for your EFR32 port (e.g., COM5)
SERIAL_PORT = '/dev/ttyACM0' 
BAUD_RATE = 115200   

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Status: Listening to EFR32 on {SERIAL_PORT}...")
except Exception as e:
    print(f"Error: Could not open {SERIAL_PORT}. Is the board plugged in?")
    exit()

# --- 3. MAIN FUNCTION ---
def listen_to_sensor():
    print("--- Starting Sensor Data Loop (Press Ctrl+C to stop) ---")
    
    while True:
        try:
            # A. CHECK FOR DATA
            if ser.in_waiting > 0:
                # 1. Read raw string from EFR32 (Hardware Layer)
                # Example input: "temp: 35, humid: 60, time: 12345"
                raw_line = ser.readline().decode('utf-8', errors='ignore').strip()
                
                if raw_line:
                    print(f"\n[Raw Input]: {raw_line}")

                    # 2. Parse the data (Logic Layer)
                    # We send the messy string to your parser file
                    clean_data = data_parser.parse_sensor_line(raw_line)

                    # 3. Push to Firestore (Database Layer)
                    if clean_data:
                        # Add a Server Timestamp (Good for sorting later)
                        clean_data['timestamp'] = firestore.SERVER_TIMESTAMP
                        
                        # We use .add() to create a new document with a random ID
                        # This creates a log history (Log_1, Log_2, Log_3...)
                        db.collection(u'sensor_data').add(clean_data)

                        
                        print(f" -> [Uploaded]: {clean_data}")
                        time.sleep(5)  # Small delay to avoid spamming
                    else:
                        print(" -> [Warning]: Could not parse data.")
                        
        except KeyboardInterrupt:
            print("\nStopping program...")
            ser.close()
            break
        except Exception as e:
            print(f"Error in loop: {e}")

if __name__ == "__main__":
    listen_to_sensor()