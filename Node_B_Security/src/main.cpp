#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include "painlessMesh.h"
#include <ArduinoJson.h>

#define MESH_PREFIX     "NVIDIA_MESH_NET"
#define MESH_PASSWORD   "Architecture123"
#define MESH_PORT       5555

#define RST_PIN         22   
#define SS_PIN          5    
#define LED_GREEN       2    
#define LED_RED         4    
#define BUZZER_PIN      25   
#define BUTTON_PIN      26   

MFRC522 mfrc522(SS_PIN, RST_PIN);
painlessMesh  mesh;
Scheduler     userScheduler; 

int lastButtonState = HIGH; 

// 🔴 المتغير الجديد الذي يحفظ حالة الطوارئ
bool isLockdown = false; 

void sendCardData(String cardID, bool accessGranted) {
  JsonDocument doc; 
  doc["node"] = "Node_B";
  doc["card"] = cardID;
  doc["access"] = accessGranted;

  String msg;
  serializeJson(doc, msg);
  mesh.sendBroadcast(msg);
  Serial.println("Sent to Mesh: " + msg);
}

// ----------------- مهام الإطفاء (Tasks) -----------------
Task taskResetSuccess(500, TASK_ONCE, []() {
  digitalWrite(LED_GREEN, LOW);
  noTone(BUZZER_PIN);
});

Task taskResetDenied(500, TASK_ONCE, []() {
  digitalWrite(LED_RED, LOW);
  noTone(BUZZER_PIN);
});

// ----------------- دوال التحكم بالهاردوير -----------------
void triggerSuccess() {
  digitalWrite(LED_GREEN, HIGH);
  tone(BUZZER_PIN, 1500); 
  taskResetSuccess.restartDelayed(500);
}

void triggerDenied() {
  digitalWrite(LED_RED, HIGH);
  tone(BUZZER_PIN, 300); 
  taskResetDenied.restartDelayed(500);
}

// ----------------- مهام القراءة -----------------
void checkButton() {
  int currentState = digitalRead(BUTTON_PIN);

  if (currentState == LOW && lastButtonState == HIGH) {
    Serial.println("Button Pressed!");
    
    // 🔴 التعديل: لا تفتح الباب بالزرار لو النظام في وضع الحظر
    if (isLockdown) {
      Serial.println("Access DENIED: System is in LOCKDOWN!");
      triggerDenied();
      sendCardData("BTN_CARD_99", false); 
    } else {
      triggerSuccess();
      sendCardData("BTN_CARD_99", true); 
    }
  }
  lastButtonState = currentState;
}
Task taskCheckButton(TASK_MILLISECOND * 50, TASK_FOREVER, &checkButton);

void checkRFID() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  String cardID = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    cardID += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    cardID += String(mfrc522.uid.uidByte[i], HEX);
  }
  cardID.toUpperCase();

  Serial.println("Card Detected: " + cardID);
  
  bool isAuthorizedCard = (cardID == "A347F22C"); 
  
  // 🔴 التعديل الجذري: الفتح يتطلب كارت صحيح + عدم وجود حالة حظر
  bool granted = isAuthorizedCard && !isLockdown; 

  if (granted) {
    Serial.println("Access GRANTED.");
    triggerSuccess();
  } else {
    if (isLockdown && isAuthorizedCard) {
      Serial.println("Access DENIED: Correct Card, but system is in LOCKDOWN!");
    } else {
      Serial.println("Access DENIED: Invalid Card.");
    }
    triggerDenied();
  }

  sendCardData(cardID, granted);
  mfrc522.PICC_HaltA(); 
}
Task taskCheckRFID(TASK_MILLISECOND * 200, TASK_FOREVER, &checkRFID);

// ----------------- دالة الاستقبال -----------------
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Node B Received: %s\n", msg.c_str());

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, msg);
  
  if (error) {
    Serial.print("JSON Parse Error: ");
    Serial.println(error.c_str());
    return;
  }

  // 🔴 التعديل: التقاط الأوامر الجديدة الخاصة بالـ Lockdown
  if (doc.containsKey("command")) {
    String command = doc["command"].as<String>();

    if (command == "LOCKDOWN_ON") {
      isLockdown = true;
      Serial.println("!!! NODE B: SYSTEM LOCKDOWN ACTIVATED !!!");
    } 
    else if (command == "LOCKDOWN_OFF") {
      isLockdown = false;
      Serial.println("--- NODE B: SYSTEM LOCKDOWN DEACTIVATED ---");
    }
    
    // التقاط أمر الفتح من التطبيق (لو كانت الرسالة موجهة لـ Node_B)
    else if (command == "UNLOCK" && doc["target"] == "Node_B") {
      if (!isLockdown) {
        Serial.println("App Command Executed: Door Unlocked!");
        triggerSuccess(); 
      } else {
        Serial.println("App Command Ignored: Cannot unlock during LOCKDOWN!");
        triggerDenied(); // نعطي تنبيه رفض لتطبيق الأوامر المرفوضة
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  SPI.begin();           
  mfrc522.PCD_Init();    
  
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  noTone(BUZZER_PIN);

  mesh.setDebugMsgTypes(ERROR | STARTUP);  
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);

  userScheduler.addTask(taskCheckButton);
  userScheduler.addTask(taskCheckRFID);
  userScheduler.addTask(taskResetSuccess);
  userScheduler.addTask(taskResetDenied);
  
  taskCheckButton.enable();
  taskCheckRFID.enable();

  Serial.println("Node B (Security) Started");
}

void loop() {
  mesh.update();
}