# ESP32-C3 SuperMini

The **ESP32-C3 SuperMini** is a tiny yet powerful development board built around the Espressif ESP32-C3 chip. With WiFi 802.11b/g/n and Bluetooth 5 (LE), it  is perfect for IoT projects that need reliable wireless connectivity.

Designed with a compact form factor, this board is easy to integrate into space-constrained projects. Its PCB antenna ensures stable wireless performance without needing an external antenna.

It includes both **RST** (reset) and **BOOT** buttons. The BOOT button is used to put the board into bootloader mode for uploading code, while the RST button resets the board—useful for restarting and running newly uploaded code.

The ESP32-C3 SuperMini has 13 exposed GPIO pins, with about 10 freely usable for general purposes, but some are pre-assigned for functions like the boot button and onboard LED. Pin functionality varies, with some supporting PWM, analog input (ADC), and I2C, but it's crucial to avoid using strapping pins during boot and pins used for the SPI flash.  

## Usable GPIO pins

- **General I/O and PWM:** GPIOs 0, 1, 3, 4, 5, 6, 7, and 10 offer a range of functions and are good candidates for general use.
- **ADC** GPIOs 3, 4, 5 may also be used for Analog To Digital conversion, however, GPIO5 ADC cannot be used if WiFi is active.
- **I2C:** GPIO5 is the default I2C SDA pin and can be used for I2C communication.
- **UART:** GPIO20 and GPIO21 are the default UART RX and TX pins, respectively. They can be released for GPIO in PlaformIO by adding *build_flags = -D ARDUINO_USB_CDC_ON_BOOT=1* to PlatformIO.ini.
- **Onboard LED:** The onboard LED is connected to GPIO8.
- **Boot Button:** GPIO9 is connected to the boot button and is a strapping pin.

## Pins to avoid for general use

- **Strapping Pins:** GPIO2, 8, 9 are strapping pins that affect the boot process and should be avoided for general use, especially during boot.
- **SPI Flash:** Pins used for the integrated SPI flash (like GPIO12-GPIO17 on some variants) should not be used for other purposes.

## Pin	Function
| Pin | Function |
| --- | --- |
| 3V3 | 3.3V output/input (outputs 3.3V from the onboard regulator, or it is a input for external 3.3V power supply) |
| 5V | 5V input/output (connects to the USB-C 5V or external 5V supply) |
| GND | GND pin |
| GPIO 0 | General-purpose I/O, ADC1, PWM |
| GPIO 1 | General-purpose I/O, ADC1, PWM |
| GPIO 2 | General-purpose I/O ~~ADC1~~, **Strapping Pin (Boot Mode) (avoid for general use)** |
| GPIO 3 | General-purpose I/O, PWM |
| GPIO 4 | General-purpose I/O, PWM, default SPI SCK pin |
| GPIO 5 | General-purpose I/O, PWM, default SPI MISO pin |
| GPIO 6 | General-purpose I/O, PWM, default SPI MOSI pin |
| GPIO 7 | General-purpose I/O, PWM, default SPI SS pin |
| GPIO 8 | Connected to the onboard LED (active low); **Strapping Pin (avoid for general use)**; Default I2C SDA pin |
| GPIO 9 | Connected to BOOT Button (LOW to enter bootloader), **Strapping Pin (avoid for general use)**; Default I2C SCL pin |
| GPIO 10 | General-purpose I/O, PWM |
| GPIO 20 | General-purpose I/O, PWM, default UART RX Pin, release with *build_flags = -D ARDUINO_USB_CDC_ON_BOOT=1* |
| GPIO 21 | General-purpose I/O, PWM, default UART TX Pin, release with *build_flags = -D ARDUINO_USB_CDC_ON_BOOT=1* |
