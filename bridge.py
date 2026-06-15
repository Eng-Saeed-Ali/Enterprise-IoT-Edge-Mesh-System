import os
import serial
import firebase_admin
from firebase_admin import credentials, db
import json
import time


current_dir = os.path.dirname(os.path.abspath(__file__))

# دمج المسار الحالي مع اسم ملف الـ JSON
key_path = os.path.join(current_dir, "iot-mesh-system-firebase-adminsdk-fbsvc-91c4fb75e0.json")

# إعداد الفايربيز
cred = credentials.Certificate(key_path) 
options = {'databaseURL': 'https://iot-mesh-system-default-rtdb.europe-west1.firebasedatabase.app'}
firebase_admin.initialize_app(cred, options)


TARGET_PORT = 'COM6' 

try:
    ser = serial.Serial(TARGET_PORT, 115200, timeout=1)
    print(f"--- Bridge Connected to ESP32 ({TARGET_PORT}) ---")
except Exception as e:
    print(f"--- [CRITICAL ERROR]: Could not connect to ESP32 on {TARGET_PORT} ---")
    print("Please check Device Manager for the correct COM port and update 'TARGET_PORT'.")
    exit()

def app_listener(event):
    """تسمع أوامر الموبايل من الفايربيز وتبعتها للميش عبر السلك"""
    if event.data:
        path = event.path
        if "command" in path or "manual_control" in path:
            target = "Node_C" if "Power_Node" in path else "Node_B"
            cmd = {"target": target, "command": event.data}
            ser.write((json.dumps(cmd) + "\n").encode())
            print(f">>> Sent Command to Mesh: {cmd}")


db.reference('/').listen(app_listener)

print("--- Bridge is Running! Waiting for telemetry data... ---")

while True:
    try:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').strip()
            if line.startswith('{'):
                data = json.loads(line)
                node = data.get("node")
                
                
                if node == "Node_A":
                    db.reference('Environment_Node').update({
                        'temperature': data['temp'], 'humidity': data['hum'], 'gas_level': data['gas']
                    })
                elif node == "Node_C":
                    db.reference('Power_Node').update({'voltage': data['volt']})
                elif node == "Node_B":
                    db.reference('Security_Node').update({
                        'last_card_id': data['card'], 'access_granted': data['access']
                    })
                print(f"[OK] Uploaded {node} data to Cloud")
    except Exception as e:
        
        
        pass
    time.sleep(0.1)