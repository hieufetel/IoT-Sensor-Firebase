import time
import random
import firebase_admin
from firebase_admin import credentials
from firebase_admin import firestore
import data_parser  # Your existing parser file

# --- CONFIGURATION ---
# Set this to TRUE to simulate data, FALSE to use real EFR32
USE_SIMULATOR = True 

# --- 1. SETUP FIRESTORE ---
# (Standard setup - same as before)
if not firebase_admin._apps:
    cred = credentials.Certificate("serviceAccountKey.json")
    firebase_admin.initialize_app(cred)
db = firestore.client()
print("Status: Firestore Connected.")

# --- 2. THE "VIRTUAL" SENSOR ---
class MockSerial:
    """
    This class pretends to be the serial.Serial library.
    It generates fake data instead of reading from USB.
    """
    def __init__(self):
        self.start_time = time.time()
        print("--- SIMULATION MODE: Virtual EFR32 Started ---")

    @property
    def in_waiting(self):
        # Pretend we always have data ready every 2 seconds
        # (This creates a simple delay so we don't spam Firebase)
        time.sleep(2) 
        return 1

    def readline(self):
        # GENERATE RANDOM DATA
        # We match your C format: "temp: %.2f, humid: %.2f, time: %lu\n"
        
        sim_temp = round(random.uniform(20.0, 35.0), 2)  # Random float 20-35
        sim_humid = round(random.uniform(40.0, 90.0), 2) # Random float 40-90
        sim_ticks = int((time.time() - self.start_time) * 1000) # Fake "ticks"
        
        # Create the string exactly how EFR32 sends it
        fake_packet = f"temp: {sim_temp}, humid: {sim_humid}, time: {sim_ticks}\n"
        
        # We must encode to bytes because real serial sends bytes
        return fake_packet.encode('utf-8')

    def close(self):
        print("Simulation Ended.")

# --- 3. CHOOSE SOURCE (REAL vs FAKE) ---
if USE_SIMULATOR:
    ser = MockSerial() # Use the fake class above
else:
    import serial
    # Use real hardware
    try:
        ser = serial.Serial('COM5', 115200, timeout=1)
        print("Connected to Real EFR32.")
    except Exception as e:
        print(f"Error: {e}")
        exit()

# --- 4. MAIN LOOP (Unchanged) ---
# This part is EXACTLY the same as your real main.py
# This proves that your logic works regardless of the source.
def main_loop():
    print("Starting Data Loop...")
    while True:
        try:
            if ser.in_waiting > 0:
                # 1. Read (Mock or Real)
                raw_line = ser.readline().decode('utf-8').strip()
                
                if raw_line:
                    print(f"\n[Incoming]: {raw_line}")

                    # 2. Parse (Your Logic)
                    clean_data = data_parser.parse_sensor_line(raw_line)

                    # 3. Push to Firebase
                    if clean_data:
                        clean_data['timestamp'] = firestore.SERVER_TIMESTAMP
                        db.collection(u'sensor_data').add(clean_data)
                        print(f" -> [Firebase]: Uploaded {clean_data}")
                        time.sleep(1)  # Avoid spamming
                    else:
                        print(" -> [Error]: Parser failed.")
                        
        except KeyboardInterrupt:
            ser.close()
            break

if __name__ == "__main__":
    main_loop()