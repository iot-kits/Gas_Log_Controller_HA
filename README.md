# Gas Log Controller

The Gas Log Controller uses an ESP32-C3, a DS20b18 temperature sensor, and a DRV8877 H-Bridge to thermostatically control the gas valve of fireplace logs through a websocket user interface. It was developed on the PlaformIO extension of VS Code as an Arduino application. The software can be updated by direct USB connection to the ESP32-C3 or wirelessly over a local Wi-Fi connection. Multicast DNS (mDNS) permits access to the controller with a friendly name.

## User Interface
<img width="374" height="491" alt="Gas_Log_UI" src="https://github.com/user-attachments/assets/63379475-9757-4ae5-9ebe-193f17fe546c" />

### Mode: On | Thermostat | Off

Three radio buttons set the operating mode of the controller. The gas valve is closed (no heating) in the **Off** position regardless of thermostat seetting or room temperature. In the **Thermostat** position, the thermostat controls the gas valve to maintain the set temperature. In the **On** mode the gas valve will open for heating.


| Mode           | State                    | System Status |
|----------------|--------------------------|---------------|
| **Off**        | Valve closed             | System Off    |
| **Thermostat** | maintain set temperature | Operating     |
| **On**         | Valve open               | Heating       |

## Schematic
<img width="3060" height="2310" alt="image" src="https://github.com/user-attachments/assets/441cfbeb-8972-468a-a39f-b63efa3b9986" />
External power of 7 to 8 Vdc is supplied to the PWR terminals of the H-bridge. This voltage is direcly used by the H-bridge to energize the gas valve and is directly connected to teh Vm terminals of teh H-bridge module. The ESP32-C3 is supplied with 5 Vdc from a buck converter receieving power from the external source connected to Vm.

The DS18b20 temperature sensor is supplied with 3.3 Vdc from the ESP32-C3. Data is exchanged with the ESP32-C3 over a OneWire bus. Resistor R1 provides the required pull up voltage.

Two GPIO pins on the ESP32-C3 provide digital signals to the H-bridge. Red and blue LEDs connected to teh GPIO lines display the logic signals sent to teh H-bridge as diagnostic indications.

## Planned Enhancements

1. Limit operation to 4 hours. This limit applies as long as the Power is set to ON regardless of the valve open/closed state. Reset the limit when the Power is manually cycled OFF.
2. Force Power OFF at a preset clock time, say between midnight and 8 AM.
3. Rebuild the controller with minimum components and smallest size. Use a skeleton sandwich frame.
4. Consider three installation options: 
   A. Device external to the fireplace with the temperature sensor mounted on the board, 
   B. Device inside the fireplace with a wired external temperature sensor, 
   C. Device inside the fireplace with a battery-powered Bluetooth sensor external 


Test new repo