# ZED-F9P FeatherWing USB: LEARN

Based extensively on the [Adafruit Ultimate GPS FeatherWing](https://www.adafruit.com/product/3133) and the
[SparkFun GPS-RTK2 Board](https://www.sparkfun.com/products/15136), the ZED-F9P FeatherWing USB takes all the great features of the GPS-RTK2 and squeezes them
into Adafruit FeatherWing format.

![Assembly_9](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Assembly_9.JPG)

The board can be used stand-alone, powered and interfaced through the USB connector:

![USB_Power](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/USB_Power.JPG)

However, it is designed to mount on top of the [Adafruit Feather M0 Adalogger](https://www.adafruit.com/products/2796)
to form a very compact dual-band PPK data logger or RTK system, especially when coupled with:
- A [SparkFun Bluetooth Mate Gold](https://www.sparkfun.com/products/12580) to provide the Real Time Correction Message link
- A [RAWX_Logger_Switches board](https://github.com/PaulZC/RAWX_Logger_Switches) to provide on/off, base/rover, survey_in, event and stop-logging buttons plus a NeoPixel for visual feedback

![Kit](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Kit.JPG)

![Stack](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Stack.JPG)

![Adalogger](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Adalogger.JPG)

![Dimensions](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Dimensions.PNG)

![Top](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Top.JPG)

![Bottom](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Bottom.JPG)

### Power Options

![Power](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Power.JPG)

The FeatherWing can be powered through:
- The micro-USB connector
- The standard FeatherWing LiPo battery and VBUS (USB) pins

The average current draw is approximately 140mA in Survey_In mode when powering the Adalogger, SparkFun Bluetooth Mate Gold and a Taoglas A.80.A.101111 antenna. Select your LiPo battery capacity accordingly!

The board has its own regulator which will regulate battery or USB power down to 3.3V.

The 3V3 pin can be connected by soldering the 3V3 split pad. Only do this if you want to use the pin as a 3.3V output. Don't solder the split pad if you will be mounting the board on a Feather,
the 3.3V regulators will fight each other and bad things may happen!

The power regulator circuit was gratefully plagiarised from the design for the [Adafruit AirLift FeatherWing](https://www.adafruit.com/product/4264).

You can disable the regulator and power down the board by pulling the EN pin low.

![EN](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/EN.JPG)

### USB

The ZED-F9P's USB pins are connected to the micro-USB connector making it easy to talk to the board using [u-center](https://www.u-blox.com/en/product/u-center).

### UART1 Pins

![UART1](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/UART1.JPG)

The ZED-F9P's UART1 serial Tx and Rx are connected to the standard Feather Serial1 Rx and Tx pins.

You can isolate the Feather pins if you need to by cutting the TX and RX links.

### UART2 Pins

![UART2](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/UART2.JPG)

The ZED-F9P's UART2 serial Tx and Rx pins are available on the six pin header on the edge of the board. The pin-out matches the pin-out of the
[SparkFun Bluetooth Mate Gold](https://www.sparkfun.com/products/12580), the Mate can be stacked on top of the board to provide an RTK RTCM link.

### I2C pins

![I2C](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/I2C.JPG)

The Zed-F9P's I2C pins are conected to the standard Feather SCL and SDA pins. Pull-up resistors are included (R3 and R4).

You can isolate the Feather pins if you need to by cutting the SDA and SCL links; you can use the extra pins in the center of the board instead.

### SAFEBOOT

![Safeboot](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Safeboot.JPG)

The ZED-F9P's SAFEBOOT pin is broken out on a test pad on the bottom of the board. You will need access to this if you want to upgrade the ZED-F9P's firmware.

### SPI

Unfortunately, the FeatherWing does not provide access to the ZED-F9P's DSEL pin - there wasn't room! So, you cannot put the ZED-F9P into SPI mode.

### Antenna

![ANTENNA](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/ANTENNA.JPG)

The FeatherWing includes mounting pads for a full size, robust, edge-launch SMA connector. L1, R2 and C3 provide 3.3V power for the required active antenna.

### EXTINT

![EXTINT](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/EXTINT.JPG)

The ZED-F9P's EXTINT (External Interrupt) pin is linked to the Feather A2 (D16) pin. By pushing the EVENT switch on the RAWX_Logger_Switches board, you can make the ZED-F9P
generate a TIM_TM2 message which can be logged to SD card and extracted with RTKLIB. The RAWX_Logger_F9P code uses A2 as an interrupt to make the NeoPixel on the Switches board flash white.

You can isolate the Feather A2 pin by cutting the INT link.

## Licence

Please see [LICENSE.md](./LICENSE.md) for details.

Enjoy!

**_Paul_**