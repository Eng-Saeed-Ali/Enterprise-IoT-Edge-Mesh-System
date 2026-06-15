# 🌐 Scalable Enterprise IoT Edge-Mesh Infrastructure

![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Python](https://img.shields.io/badge/Python-3776AB?style=for-the-badge&logo=python&logoColor=white)
![Firebase](https://img.shields.io/badge/Firebase-FFCA28?style=for-the-badge&logo=firebase&logoColor=black)
![PlatformIO](https://img.shields.io/badge/PlatformIO-F58220?style=for-the-badge&logo=platformio&logoColor=white)

> **Author:** Saeed Ali
> **Role:** Software & Embedded Systems Engineer

## 📌 Executive Summary
This repository contains the complete software architecture and data pipeline for a decentralized, highly scalable wireless sensor network. Designed as a commercial-grade prototype, it bridges the gap between low-level hardware instrumentation and high-level enterprise cloud applications.

The system deliberately bypasses traditional microcontroller bottlenecks (such as RAM exhaustion during SSL/TLS handshakes) by implementing a strict **Separation of Concerns**. By offloading heavy cryptographic network tasks to a dedicated Python middleware (Edge Gateway), the edge nodes are freed to achieve millisecond-latency telemetry, self-healing routing, and real-time execution.

## 💼 Commercial Use Cases
This architecture is built with commercial scalability in mind, making it directly applicable to:
* **Smart Warehousing:** Decentralized monitoring of temperature/humidity across vast spaces without the need for extensive Wi-Fi coverage.
* **Server Room Infrastructure:** Real-time environmental tracking coupled with immediate biometric access lockdown protocols.
* **Industrial Automation:** Remote machine actuation and power consumption monitoring with instant state-feedback.

## 🏗️ System Architecture

The project is structured into three decoupled layers:

### 1. The Edge Mesh Network (C++ Firmware)
A completely decentralized, self-healing local network using the `painlessMesh` protocol. Nodes communicate peer-to-peer, ensuring high availability even if local Wi-Fi fails.
* **Master Node (Coordinator):** Acts as the high-speed serial bridge between the wireless mesh and the wired gateway, handling asynchronous data streams.
* **Node A (Environmental):** Monitors ambient conditions (DHT22 & MQ-135). Executes an autonomous **Emergency Protocol** to broadcast mesh-wide priority alerts and trigger physical alarms if hazardous gas thresholds are breached.
* **Node B (Security & Access):** Manages local RFID biometric authentication. Processes network-wide `LOCKDOWN` payloads to override physical access instantly, actively denying valid RFID cards until the lockdown is lifted remotely.
* **Node C (Power Management):** Executes remote actuation via relays and tracks raw current draw, pushing real-time JSON state-feedback upon command execution for immediate UI syncing.

### 2. The Edge Gateway (Python Middleware)
A critical business-logic component that acts as the nervous system connecting the local serial mesh to the Cloud.
* Utilizes `firebase_admin.db.reference.listen` for asynchronous, event-driven monitoring of incoming cloud commands.
* Injects commands instantly into the Serial pipeline, resulting in near-zero latency execution.
* Ensures 100% hardware stability by handling all complex JSON parsing and Firebase communication logic off-chip.

### 3. Cloud Pipeline & Frontend Integration
* **Backend:** Firebase Realtime Database handles bidirectional JSON data synchronization.
* **Frontend Dashboard:** The architecture is designed to feed a decoupled cross-platform application. The mobile frontend consumes the Firebase WebSockets, enabling administrators to visualize live telemetry, trigger facility lockdowns, and toggle relays remotely. *(Note: The frontend operates as an independent consumer of this backend architecture).*

## ⚙️ Engineering Highlights
* **Strictly Non-Blocking Software:** Zero `delay()` functions. The entire C++ firmware relies on hardware timers, `TaskScheduler`, and asynchronous callbacks to maintain mesh integrity.
* **Dynamic JSON Data Pipelines:** Efficient parsing and serialization of structured telemetry data across all nodes and the gateway using `ArduinoJson v7` and Python's `json` library.
* **Fault Tolerance & Edge Cases:** Robust logic to handle sensor failures gracefully (e.g., isolating DHT22 crashes to ensure gas monitoring remains online without crashing the node).

## 🚀 Getting Started

### Development Environment
* Firmware developed, optimized, and compiled using **PlatformIO**.
* Python scripts tested on Python 3.x environments.

### Installation & Setup
1. **Flash the Nodes:**
   Open the C++ source code in PlatformIO. Ensure the `painlessMesh`, `ArduinoJson`, and sensor libraries are installed. Flash the specific node `.cpp` files to your microcontrollers.
2. **Setup the Middleware:**
   ```bash
   pip install pyserial firebase-admin
3. **Configure the Bridge:**
   Update the TARGET_PORT in bridge.py to match your Master Node's serial port. Place your Firebase Admin SDK private key JSON in the project root directory.
4. **Run the Gateway:**
   **Bash**
   python bridge.py
   **Successful connection will output: --- Bridge Connected to ESP32 ---**

Building robust, commercial-ready software architectures from the silicon up.