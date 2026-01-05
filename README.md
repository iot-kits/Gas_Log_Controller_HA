# Gas Log Controller

The Gas Log Controller uses an ESP32-C3 and a DRV8877 H-Bridge to thermostatically control the gas valve of fireplace logs through a websocket uswr interface. It was developed on the PlaformIO extension of VS Code as an Arduino application. The software can be updated by direct USB connection to the ESP32-C3 or wirlessly over a local Wi-Fi connection.

## User Interface

<img width="314" height="427" alt="Gas_Log_UI" src="https://github.com/user-attachments/assets/eaabd6fc-885e-4523-b774-e6840037d7d5" />

### Power: ON | OFF

The power buttons turn the controller on and off. The gas valve is closed (no heating) in the OFF position regardless of Mode. In the ON position, the thermostat controls the valve in the AUTOMATIC mode and the valve will open for heating in the MANUAL position.


| Power \ Mode | AUTOMATIC  | MANUAL     |
|--------------|------------|------------|
| **ON**       | Thermostat | Heating    |
| **OFF**      | System OFF | System OFF |

## Schematic




## Planned Enhancements

1. Set a color for the room temperature display background: HEATING | IDLE | OFF 
2. Limit operation to 4 hours. This limit applies as long as the Power is set to ON regardless of the valve open/closed state. Reset the limit when the Power is manually cycled OFF.
3. Force Power OFF at a preset clock time, say between midnight and 8 AM.
4. Rebuild the controller with minimum components and smallest size. Use a skeleton sandwich frame.5. Consider three installation options: 
   A. Device external to the fireplace with the temperature sensor mounted on the board, 
   B. Device inside the fireplace with a wired external temperature sesnor, 
   C. Device inside the fireplace with a battery-powered Bluetooth sensor external 