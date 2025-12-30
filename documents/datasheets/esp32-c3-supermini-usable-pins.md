# ESP32-C3 SuperMini

The ESP32-C3 SuperMini has 13 exposed GPIO pins, with about 10 freely usable for general purposes,  
but some are pre-assigned for functions like the boot button and onboard LED. Pin functionality varies,  
with some supporting PWM, analog input (ADC), and I2C, but it's crucial to avoid using strapping pins  
during boot and pins used for the SPI flash.  

## Usable GPIO pins

- **General I/O and PWM:** GPIOs 0, 1, 3, 4, 5, 6, 7, and 10 offer a range of functions and are good candidates for general use.
- **ADC** GPIOs 3, 4, 5 may also be used fo Analog To Digital conversion.
- **I2C:** GPIO5 is the default I2C SDA pin and can be used for I2C communication.
- **UART:** GPIO20 and GPIO21 are the default UART RX and TX pins, respectively.
- **Onboard LED:** The onboard LED is connected to GPIO8.
- **Boot Button:** GPIO9 is connected to the boot button and is a strapping pin.

## Pins to avoid for general use

- **Strapping Pins:** GPIO2, 8, 9 are strapping pins that affect the boot process and should be avoided for general use, especially during boot.
- **SPI Flash:** Pins used for the integrated SPI flash (like GPIO12-GPIO17 on some variants) should not be used for other purposes.
