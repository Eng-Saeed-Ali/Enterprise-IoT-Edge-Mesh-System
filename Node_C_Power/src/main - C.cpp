#include <Arduino.h>
#include "painlessMesh.h"
#include <ArduinoJson.h>

// --- إعدادات شبكة الميش (نفس البيانات لضمان الاتصال) ---
#define MESH_PREFIX     "NVIDIA_MESH_NET"
#define MESH_PASSWORD   "Architecture123"
#define MESH_PORT       5555

// --- التوصيلات ---
#define RELAY1_PIN      26   
#define RELAY2_PIN      27   
#define CURRENT_PIN     32   

painlessMesh  mesh;
Scheduler     userScheduler; 

bool relay1State = false; 
bool relay2State = false;

// دالة إرسال البيانات للماستر (فولت وحالة الريليهات)
void sendPowerStatus() {
  int rawCurrent = analogRead(CURRENT_PIN);
  float sensorVoltage = (rawCurrent / 4095.0) * 3.3;

  JsonDocument doc; 
  doc["node"] = "Node_C";
  doc["current_raw"] = rawCurrent;
  doc["volt"] = sensorVoltage;
  doc["relay1"] = relay1State ? "ON" : "OFF";
  doc["relay2"] = relay2State ? "ON" : "OFF";

  String msg;
  serializeJson(doc, msg);
  mesh.sendBroadcast(msg);
  
  Serial.println("Broadcasted Power Status: " + msg);
}

// مهمة الإرسال الدوري كل 5 ثواني
Task taskSendStatus(TASK_SECOND * 5, TASK_FOREVER, &sendPowerStatus);

// --- دالة الاستقبال (معدلة للتأكد من تنفيذ أوامر الأبلكيشن والماستر) ---
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Node C Received from %u: %s\n", from, msg.c_str());
  
  JsonDocument doc; 
  DeserializationError error = deserializeJson(doc, msg);
  
  if (error) {
    Serial.println("JSON Parse Error");
    return; 
  }

  // التأكد إن الأمر مبعوت لـ Node_C
  if (doc["target"] == "Node_C") {
    String command = doc["command"].as<String>();

    // التحكم في ريلاي 1 (المروحة / الجهاز الأساسي)
    if (command == "R1_ON") {
      digitalWrite(RELAY1_PIN, LOW); // تشغيل (Active Low)
      relay1State = true;
      Serial.println(">>> Action: Relay 1 Turned ON via Command");
    } 
    else if (command == "R1_OFF") {
      digitalWrite(RELAY1_PIN, HIGH); // إيقاف
      relay1State = false;
      Serial.println(">>> Action: Relay 1 Turned OFF via Command");
    }

    // التحكم في ريلاي 2
    if (command == "R2_ON") {
      digitalWrite(RELAY2_PIN, LOW);
      relay2State = true;
      Serial.println(">>> Action: Relay 2 Turned ON via Command");
    } 
    else if (command == "R2_OFF") {
      digitalWrite(RELAY2_PIN, HIGH);
      relay2State = false;
      Serial.println(">>> Action: Relay 2 Turned OFF via Command");
    }
    
    // إرسال تحديث فوري للحالة بعد تنفيذ الأمر عشان الأبلكيشن يحس بالتغيير فوراً
    sendPowerStatus();
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  
  // البداية بوضع الإيقاف
  digitalWrite(RELAY1_PIN, HIGH); 
  digitalWrite(RELAY2_PIN, HIGH);

  mesh.setDebugMsgTypes( ERROR | STARTUP );  
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);

  userScheduler.addTask(taskSendStatus);
  taskSendStatus.enable();

  Serial.println("Node C (Power & Actuation) Started");
}

void loop() {
  mesh.update();
}