#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "painlessMesh.h"
#include <ArduinoJson.h>


#define MESH_PREFIX     "NVIDIA_MESH_NET"
#define MESH_PASSWORD   "Architecture123"
#define MESH_PORT       5555

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
#define BUTTON_PIN 15


int screenPage = 0; 
float a_temp = 0.0, a_hum = 0.0; int a_gas = 0; bool a_alert = false;
float c_volt = 0.0; String c_relay1 = "OFF", c_relay2 = "OFF";
String b_card = ""; 
bool b_access = false; 
bool systemLockdown = false; 
bool showNodeBAlert = false; 
uint32_t alertStartTime = 0;

painlessMesh  mesh;
Scheduler     userScheduler; 


String serialBuffer = "";
uint32_t lastButtonPress = 0;

void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(WHITE);

  if (showNodeBAlert) {
    display.setTextSize(2); display.println("DOOR ALERT");
    display.setTextSize(1); display.println("---------------------");
    display.print("ID: "); display.println(b_card);
    
    if (systemLockdown) {
       display.setCursor(0, 45);
       display.setTextSize(2);
       display.println("LOCKED!"); 
    } else {
       display.setCursor(0, 48);
       display.setTextSize(1);
       if (b_access) display.println("ACCESS: GRANTED");
       else display.println("ACCESS: DENIED");
    }
  } 
  else {
    if (screenPage == 0) {
      display.println("Node: Node_A (Env)");
      display.println("---------------------");
      display.printf("Temp: %.1f C\nHum:  %.1f %%\nGas:  %d\n", a_temp, a_hum, a_gas);
    } else {
      display.println("Node: Node_C (Power)");
      display.println("---------------------");
      display.printf("Volt: %.2f V\nR1: %s | R2: %s\n", c_volt, c_relay1.c_str(), c_relay2.c_str());
    }
    
    display.setCursor(0, 56);
    if(systemLockdown) display.print("[!] SYSTEM LOCKED");
    else display.print("[ ] System Normal");
  }
  display.display();
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.println(msg); 
  JsonDocument doc;
  if (deserializeJson(doc, msg)) return;

  String nodeName = doc["node"].as<String>();
  if (nodeName == "Node_A") {
    a_temp = doc["temp"]; a_hum = doc["hum"]; a_gas = doc["gas"]; a_alert = doc["alert"];
  } 
  else if (nodeName == "Node_C") {
    c_volt = doc["volt"]; c_relay1 = doc["relay1"].as<String>(); c_relay2 = doc["relay2"].as<String>();
  } 
  else if (nodeName == "Node_B") {
    b_card = doc["card"].as<String>();
    b_access = doc["access"].as<bool>();
    showNodeBAlert = true; 
    alertStartTime = millis(); 
  }
  updateDisplay();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setCursor(0, 20);
  display.setTextColor(WHITE);
  display.println("BRIDGE MASTER READY");
  display.display();

  mesh.setDebugMsgTypes(ERROR | STARTUP);  
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  systemLockdown = false; 
}

void loop() {
  mesh.update(); 

  
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n') {
      serialBuffer.trim(); 
      if (serialBuffer.indexOf("{") >= 0) {
        JsonDocument cmdDoc;
        DeserializationError err = deserializeJson(cmdDoc, serialBuffer);
        if(!err) {
          String cmd = cmdDoc["command"].as<String>();
          if (cmd == "LOCKDOWN_ON") {
              systemLockdown = true;
              Serial.println(">>> MASTER: MODE LOCKED");
          }
          else if (cmd == "LOCKDOWN_OFF") {
              systemLockdown = false;
              Serial.println(">>> MASTER: MODE NORMAL");
          }
          
          mesh.sendBroadcast(serialBuffer); 
          updateDisplay();
        }
      }
      serialBuffer = ""; 
    } else {
      serialBuffer += c;
    }
  }


  if (digitalRead(BUTTON_PIN) == LOW) {
    if (millis() - lastButtonPress > 300) { // 300ms Debounce
      screenPage = !screenPage;
      if (!showNodeBAlert) updateDisplay();
      lastButtonPress = millis();
    }
  }


  if (showNodeBAlert && (millis() - alertStartTime > 4000)) {
    showNodeBAlert = false;
    updateDisplay();
  }
}