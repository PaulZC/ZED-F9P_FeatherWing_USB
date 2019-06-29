# ZED-F9P FeatherWing USB

A GNSS FeatherWing incorporating the dual-band (L1 + L2) [u-blox ZED-F9P](https://www.u-blox.com/en/product/zed-f9p-module)

The board can be used stand-alone, powered and interfaced through the USB connector:

![USB_Power](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/USB_Power.JPG)

However, it is designed to mount on top of the [Adafruit Feather M0 Adalogger](https://www.adafruit.com/products/2796)
to form a very compact dual-band PPK data logger or RTK system:

![Stack](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Stack.JPG)

This version provides access to the ZED-F9P's USB port. The [ZED-F9P_FeatherWing](https://github.com/PaulZC/ZED-F9P_FeatherWing) is a simpler version
with fewer components and a CR1220 back-up battery but without USB.

Based extensively on the [Adafruit Ultimate GPS FeatherWing](https://www.adafruit.com/product/3133) and the
[SparkFun GPS-RTK2 Board](https://www.sparkfun.com/products/15136)

If you don't want to make your own FeatherWing and want to use the SparkFun GPS-RTK2 Board instead, you can find a tutorial showing you how to do that
[here](https://github.com/PaulZC/F9P_RAWX_Logger)

See [LEARN.md](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/LEARN.md) for more details

[ASSEMBLY.md](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/ASSEMBLY.md) contains instructions on how to assemble the PCB

[PAIRING.md](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/PAIRING.md) contains instructions on how to configure two SparkFun Bluetooth Mate Gold boards
as a bonded pair, to provide a 115200 Baud RTCM link

The Eagle files are available in the [Eagle folder](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/tree/master/Eagle)

The schematic is available [here](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Schematic.PNG)

The BOM is available [here](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/BOM.pdf)

The PCB thickness should be 0.8mm: see the microstrip calculations on the [SparkFun schematic](https://cdn.sparkfun.com/assets/9/a/0/0/a/Qwiic_GPS-RTK2_-_ublox_ZED-F9P.pdf)

## Arduino Code

Two versions of the Arduino code can be found in the [Arduino folder](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/tree/master/Arduino):

- **RAWX_Logger_F9P** is very similar to the logger I wrote for the [NEO-M8T FeatherWing](https://github.com/PaulZC/NEO-M8T_GNSS_FeatherWing) but is substantially updated for the ZED-F9P.
It does all of the message configuration via UART1.

- **RAWX_Logger_F9P_I2C** is a little more experimental. It uses the SparkFun Ublox library to do all of the message configuration via the I2C port instead of UART1. This makes
the code more efficient as it does not need to ignore the UBX ACK messages (on UART1) and can simply stream all data from UART1 to the SD card. The code is only experimental
as it uses sendCommand to send a custom ubxPacket and the ACK gets ignored as the [code expects it to be in a different place](https://github.com/sparkfun/SparkFun_Ublox_Arduino_Library/issues/23).
The code does work nicely though and I do prefer it over the UART-only version.

You can find instructions on how to install the Arduino IDE and the required libraries [here](https://github.com/PaulZC/F9P_RAWX_Logger/blob/master/SOFTWARE.md).

The code has three 'modes': Rover, Base and Survey_In. The mode is set by pins A0 and A3 via the [RAWX Logger Switches](https://github.com/PaulZC/RAWX_Logger_Switches). The RAWX
messages are logged to SD card in all three modes for post-processing with [rtklibexplorer's](https://rtklibexplorer.wordpress.com/) version of [RTKLIB](http://rtkexplorer.com/downloads/rtklib-code/).

You will find settings in the code to help with debugging should you need it. Uncomment the line which contains **#define DEBUG** to enable them.

## Acknowledgements

This project would not have been possible without the open source designs kindly provided by both Adafruit and SparkFun. Their work is gratefully acknowledged.

Adafruit invests time and resources providing this open source design, please support Adafruit and open-source hardware by purchasing products from Adafruit!

Feel like supporting open source hardware? Buy a board from SparkFun!

## Licence

This project is distributed under a Creative Commons Attribution + Share-alike (BY-SA) licence.
Please refer to section 5 of the licence for the "Disclaimer of Warranties and Limitation of Liability".

Enjoy!

**_Paul_**

