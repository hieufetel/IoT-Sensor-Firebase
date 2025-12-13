# --- 1. SETUP FIRESTORE ---
cred = credentials.Certificate("serviceAccountKey.json")
firebase_admin.initialize_app(cred)
db = firestore.client()
print("Status: Firestore Connected.")

# --- 2. SETUP SERIAL ---
SERIAL_PORT = '/dev/ttyACM0' 
BAUD_RATE = 115200   

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Status: Listening to EFR32 on {SERIAL_PORT}...")
except Exception as e:
    print(f"Error: Could not open {SERIAL_PORT}. Is the board plugged in?")
    exit()
