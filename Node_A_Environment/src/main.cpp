#include <Arduino.h>
#include "painlessMesh.h"
#include <ArduinoJson.h>
#include <DHT.h>

// --- إعدادات شبكة الميش ---
#define MESH_PREFIX     "NVIDIA_MESH_NET"
#define MESH_PASSWORD   "Architecture123"
#define MESH_PORT       5555

// --- إعدادات الهاردوير لـ Node A ---
#define DHTPIN 4          // حساس الحرارة متوصل بـ D4
#define DHTTYPE DHT22
#define MQ135_PIN 34      // حساس الغاز متوصل بـ D34
#define BUZZER_PIN 25     // البزر متوصل بـ D25

DHT dht(DHTPIN, DHTTYPE);
painlessMesh  mesh;
Scheduler     userScheduler; 

// --- دالة قراءة الحساسات وإرسالها ---
void sendSensorData() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  
  // قراءة مستوى الغاز (0 - 4095)
  int gasLevel = analogRead(MQ135_PIN);

  // منطق الإنذار المحلي
  bool isGasAlert = false;
  if (gasLevel > 2000) { // الحد الخطر للغاز
    digitalWrite(BUZZER_PIN, HIGH);
    isGasAlert = true;
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // حماية ضد توقف النظام لو حساس الحرارة فصل
  if (isnan(temp) || isnan(hum)) {
    Serial.println("Warning: Error reading DHT22! Sending Gas data only.");
    temp = 0.0; 
    hum = 0.0;
  }

  // تغليف البيانات في JSON (تحديث V7)
  JsonDocument doc; 
  doc["node"] = "Node_A";
  doc["temp"] = temp;
  doc["hum"] = hum;
  doc["gas"] = gasLevel;
  doc["alert"] = isGasAlert;

  // إرسال للشبكة
  String msg;
  serializeJson(doc, msg);
  mesh.sendBroadcast(msg);
  
  Serial.println("Broadcasted: " + msg);
}

// إرسال البيانات كل 5 ثواني
Task taskSendData(TASK_SECOND * 5, TASK_FOREVER, &sendSensorData);

void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Node A Received from %u: %s\n", from, msg.c_str());
}

void setup() {
  Serial.begin(115200);

  // تهيئة الحساسات
  dht.begin();
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(MQ135_PIN, INPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // تهيئة الميش
  mesh.setDebugMsgTypes( ERROR | STARTUP );  
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);

  // تشغيل مهمة الإرسال
  userScheduler.addTask(taskSendData);
  taskSendData.enable();

  Serial.println("\n--- Node A (Environment) is ONLINE & Connected ---");
}

void loop() {
  mesh.update(); // السطر ده الأهم في الميش، إياك تحط Delay هنا!
}