import socket
import firebase_admin
from firebase_admin import credentials, firestore
import data_parser

# -----------------------------------
# 1. FIREBASE SETUP
# -----------------------------------
cred = credentials.Certificate("serviceAccountKey.json")
firebase_admin.initialize_app(cred)
db = firestore.client()

print("Status: Firestore Connected.")

# -----------------------------------
# 2. UDP SETUP
# -----------------------------------
UDP_IP = "127.0.0.1"
UDP_PORT = 5006

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"Listening UDP on {UDP_PORT}...")

# -----------------------------------
# 3. MAIN LOOP
# -----------------------------------
while True:
    data, addr = sock.recvfrom(1024)
    raw_text = data.decode('utf-8', errors='ignore').strip()

    if not raw_text:
        continue

    print(f"[UDP RX] {raw_text}")

    # Parse chuỗi vừa RX
    parsed = data_parser.parse_sensor_line(raw_text)

    if parsed:
        parsed['timestamp'] = firestore.SERVER_TIMESTAMP
        db.collection("sensor_data").add(parsed)
        print(f"  -> Uploaded to Firebase: {parsed}")
    else:
        print("  -> Cannot parse!")
