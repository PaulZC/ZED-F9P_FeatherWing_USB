# ZED-F9P FeatherWing USB

A GNSS FeatherWing incorporating the dual-band (L1 + L2) [u-blox ZED-F9P](https://www.u-blox.com/en/product/zed-f9p-module)

The board can be used stand-alone, powered and interfaced through the USB connector:

![USB_Power](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/USB_Power.PNG)

However, it is designed to mount on top of the [Adafruit Feather M0 Adalogger](https://www.adafruit.com/products/2796)
to form a very compact dual-band PPK data logger or RTK system:

![Stack](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Stack.PNG)

This version provides access to the ZED-F9P's USB port. The [ZED-F9P_FeatherWing](https://github.com/PaulZC/ZED-F9P_FeatherWing) is a simpler version
with fewer components and a CR1220 back-up battery but without USB.

Based extensively on the [Adafruit Ultimate GPS FeatherWing](https://www.adafruit.com/product/3133) and the
[SparkFun GPS-RTK2 Board](https://www.sparkfun.com/products/15136)

**See [LEARN.md](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/LEARN.md) for more details**

[ASSEMBLY.md](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/ASSEMBLY.md) contains instructions on how to assemble the PCB

The Eagle files are available in the [Eagle folder](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/tree/master/Eagle)

The schematic is available [here](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Schematic.PNG)

The BOM is available [here](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/BOM.pdf)

PCB thickness should be 0.8mm: see the microstrip calculations on the [SparkFun schematic](https://cdn.sparkfun.com/assets/9/a/0/0/a/Qwiic_GPS-RTK2_-_ublox_ZED-F9P.pdf)

## Acknowledgements

This project would not have been possible without the open source designs kindly provided by both Adafruit and SparkFun. Their work is gratefully acknowledged.

Adafruit invests time and resources providing this open source design, please support Adafruit and open-source hardware by purchasing products from Adafruit!

Feel like supporting open source hardware? Buy a board from SparkFun!

## Licence

This project is distributed under a Creative Commons Attribution + Share-alike (BY-SA) licence.
Please refer to section 5 of the licence for the "Disclaimer of Warranties and Limitation of Liability".

Enjoy!

**_Paul_**

