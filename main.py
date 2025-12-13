import serial
import time
import firebase_admin
from firebase_admin import credentials
from firebase_admin import firestore
from datetime import datetime  # <--- [MỚI] Thêm thư viện xử lý thời gian

# --- IMPORT YOUR CUSTOM PARSER ---
import data_parser

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

# --- 3. MAIN FUNCTION ---
def listen_to_sensor():
    print("--- Starting Sensor Data Loop (Press Ctrl+C to stop) ---")
    
    while True:
        try:
            # A. CHECK FOR DATA
            if ser.in_waiting > 0:
                raw_line = ser.readline().decode('utf-8', errors='ignore').strip()
                
                if raw_line:
                    print(f"\n[Raw Input]: {raw_line}")

                    # 2. Parse data
                    clean_data = data_parser.parse_sensor_line(raw_line)

                    # 3. Push to Firestore
                    if clean_data:
                        # Vẫn giữ cái này để App dễ sort (sắp xếp)
                        clean_data['timestamp'] = firestore.SERVER_TIMESTAMP
                        
                        # --- [MỚI] TẠO ID ĐẸP (CLEAN ID) ---
                        # Tạo chuỗi ID ví dụ: "2023-12-14_15-30-05"
                        custom_id = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
                        
                        # Dùng .document(ID).set() thay vì .add()
                        # Để ghi vào đúng cái ID ngày giờ mình vừa tạo
                        db.collection(u'sensor_data').document(custom_id).set(clean_data)
                        
                        print(f" -> [Uploaded]: {clean_data} (ID: {custom_id})")
                        
                        # Delay 5s để không bị trùng giây (vì ID tính theo giây)
                        time.sleep(1) 
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