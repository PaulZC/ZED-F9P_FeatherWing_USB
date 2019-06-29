# ZED-F9P FeatherWing USB: Bluetooth Mate Pairing

The ZED-F9P FeatherWing has been designed so that a [SparkFun Bluetooth Mate Gold](https://www.sparkfun.com/products/12580) can be stacked on top of the FeatherWing
to provide a 115200 Baud Real Time Correction Message link.

If you are going to use two FeatherWing systems to form a Base and Rover pair, you will need to pair the Bluetooth Mates with each other so that they automatically
connect to each other on power up (without needing any extra 'connect' commands).

You can find instructions on how to configure the Bluetooth Mate on the [SperkFun website](https://learn.sparkfun.com/tutorials/using-the-bluesmirf/all). I have found that
configuring the Mates as Master and Slave works up to a point, but the Mates don't automatically reconnect after the Slave has been powered off and on again. I have found
that the best solution is to configure them as a bonded pair.

I've tested the following on RN41s programmed with Version 6.15 (04/26/2013) firmware.

You will need to know the MAC ID's of both boards. You can either read this off the sticker on top of the RN41 chip, or use the 'D' command via the serial monitor.
It is a 12 digit number starting with 000666.

You can use the Adalogger as a USB to serial pass-though and configure the boards using the Arduino IDE serial monitor. The
[Arduino folder](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/tree/master/Arduino) contains a sketch called 'Bluetooth_Mate_Echo'. This does the same job as
SparkFun's Passthrough sketch but will also blink the Adalogger's LEDs so you can see the serial port activity.

Connect the Adalogger to the Mate as shown below:

![Pairing](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Pairing.JPG)

and then upload the Bluetooth_Mate_Echo sketch.

- Open the serial monitor, set the Baud rate to **115200** and set the line ending to 'No line ending'
- Type $$$ into the message box and click Send
- You should see the response CMD
- Change the line ending to 'Carriage return'
- Type D into the message box and click Send
- You should see the RN41's settings, including its MAC ID (BTA)
- Enter and send the following commands one at a time:
  - **SX,1** (This enables bonding)
  - **SR,nnnnnnnnnnnn** (where nnnnnnnnnnnn is the MAC ID of the RN41 you want to pair with)
  - **ST,0** (This disables the remote configuration timer)
  - **SA,0** (This disables authentication)
  - **SM,6** (This sets pairing mode)
  - **R,1** (This reboots the RN41)

Repeat this on the other Mate. For the SR command, use the MAC ID of the first mate.

When both Mates are powered up, the green connect LED should light up within a couple of seconds.

If the above does not work correctly and you have changed the settings previously, it might be best to do a factory reset. Enter command mode, send **SF,1** and then **R,1** .
The **SF,1** sets the RN41 back to the factory settings. **R,1** reboots the RN41. Enter command mode again and repeat the above.

Make sure you send the **SX** command _before_ the **SR** command. SX clears any previously stored remote addresses.

You can use the 'D' command to check if the configuration has been changed. You should see 'Mode=Pair', 'Authen=0', 'Bonded=1' and 'Rem=' should be followed by the MAC ID of the paired RN41.

## Acknowledgements

This project would not have been possible without the open source designs and tutorials kindly provided by SparkFun. Their work is gratefully acknowledged.

Feel like supporting open source hardware? Buy a board from SparkFun!

## Licence

This project is distributed under a Creative Commons Attribution + Share-alike (BY-SA) licence.
Please refer to section 5 of the licence for the "Disclaimer of Warranties and Limitation of Liability".

Enjoy!

**_Paul_**

