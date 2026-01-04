# Gas Log Controller

The Gas Log Controller uses an ESP32-C3 and a DRV8877 H-Bridge to thermostatically control the gas valve of fireplace logs through a websocket uswr interface. It was developed on the PlaformIO extension of VS Code as an Arduino application. The software can be updated by direct USB connection to the ESP32-C3 or wirlessly over a local Wi-Fi connection.

## User Interface

<img width="314" height="427" alt="Gas_Log_UI" src="https://github.com/user-attachments/assets/eaabd6fc-885e-4523-b774-e6840037d7d5" />

### Power: ON | OFF

The power buttons turn the controller on of off. The gas valve is forced closed in the OFF position. In the ON position
