# ZED-F9P FeatherWing USB Assembly Instructions

Here are the assembly instructions for the ZED-F9P FeatherWing USB:

### Blank PCB

Start by having the blank PCBs manufactured. If you are based in the UK or Europe, I can recommend
[Multi-CB](https://www.multi-circuit-boards.eu/en/index.html) who can manufacture PCBs in 1-8 working days and
can process the Eagle .brd file direct - there's no need to generate Gerber files.

![PCBs](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/PCBs.JPG)

My recommended options are:
- Layers: 2 layers
- Format: single pieces
- Surface finish: chemical gold (ENIG)
- Material: FR4, 0.8mm
- Cu layers: 35um
- Solder stop: both sides, green
- Marking print: both sides, white

The 0.8mm PCB thickness is important as it defines the 50 ohm impedance of the antenna traces.

### Add solder paste

Multi-CB can also provide you with a solder paste (SMD) stencil for the PCB. My recommended options are:
- SMD stencil for: top layer
- Make the X and Y dimensions 20mm longer than the PCB itself to allow you to fix it down with tape
- Type: custom
- Pad reduction: yes, 10%
- Thickness: 100um
- Existing fiducials: lasered through
- Text type: half lasered
- Double-sided brushing: yes

I secure the blank PCB onto a flat work surface by locating it between two engineer's squares. I use a sheet of toughened glass
as the work surface as it is both very flat and easy to clean.

![Assembly_1](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Assembly_1.JPG)

Use the two round fiducials to line up the stencil with the PCB. Secure the stencil with tape.

![Assembly_2](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Assembly_2.JPG)

Apply solder paste close to the component cut-outs and then scrape the paste over the stencil using a knife blade or a similar straight edge.
Apply reasonable pressure to keep the stencil held firmly against the circuit board. Try to apply the solder paste in a single pass.
Avoid multiple passes as this will force extra paste into the spaces between the ZED-F9P's pads and could lead to shorts. If in doubt, wash the
paste off with IPA and start again. You need a lot less paste than you might think.

Take appropriate safety precautions when working with solder paste - particularly if you are using tin-lead solder instead of lead-free.

![Assembly_3](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Assembly_3.JPG)

![Assembly_4](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Assembly_4.JPG)

### Position the surface mount components

Position the components onto the blobs of solder paste using tweezers. A magnifier lamp or a USB microscope will
help you place the components in the correct position. You may find the [SMT_Component_Placer](https://github.com/PaulZC/SMT_Component_Placer) useful.

I recommend placing the ZED-F9P first, while the solder paste is fresh. Take extra time when positioning the ZED-F9P. It is an expensive chip and all of the pads are hidden.
Line it up with carefully with the white marks. You will find the OpenSCAD and STL files for a positioning jig in the [OpenSCAD folder](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/tree/master/OpenSCAD).

![Jig1](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Jig1.JPG)

![Jig2](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Jig2.JPG)

![Jig3](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Jig3.JPG)

**The lithium battery B1 is NOT reflow compatible. You will need to solder it by hand.**

![Assembly_5](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Assembly_5.JPG)

### Reflow the surface mount components

Use a reflow oven to heat the circuit board to the correct temperatures to melt the solder. A reflow oven doesn't need to be
expensive. The one shown below was assembled from:

- Quest 9L 800W mini-oven
- Inkbird PID temperature controller and 40A solid state relay
- Type K thermocouple

Several people have published good reflow oven construction guides, including [this one](http://tt7hab.blogspot.com/2018/06/the-reflow-oven.html) and [this one](http://www.die4laser.com/toaster/index.html).

![Assembly_6](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Assembly_6.JPG)

Use the correct temperature profile for your solder paste, but you won't go far wrong with setpoints of 170C for four minutes, followed by
210C for one minute, followed by a cool-down with the door open. Use a flashlight to check that the solder has melted across
the whole PCB at 210C. Hold the temperature at 210C a little longer if some of the solder paste still appears 'gray' instead of 'silver'.

![Assembly_7](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Assembly_7.JPG)

All being well, your PCB should look like this:

![Assembly_8](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Assembly_8.JPG)

### Check for shorts

Carefully examine all the solder joints and correct any shorts you find.

The 'trick' to removing a short is to add more solder or solder paste and then to use copper solder braid or wick to remove all the solder in one go.
Use good quality braid and make sure it comes pre-coated with flux. Some cheap braids don't have flux on them and don't work anywhere near as well.

Use a cotton swab dipped in Iso-Propyl Alcohol (IPA / Propanol / rubbing alcohol) to remove any flux residue.

### Add the non-surface mount components

The non-surface mount components (rechargeable battery and SMA connector) can now be soldered in by hand.

![Assembly_9](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Assembly_9.JPG)

The board can be used stand-alone and powered through the USB connector. However, it is designed to mount on top of the [Adafruit Feather M0 Adalogger](https://www.adafruit.com/products/2796)
and [stacking headers](https://www.adafruit.com/product/2830) are ideal for that.

If you are going to use a [SparkFun Bluetooth Mate Gold](https://www.sparkfun.com/products/12580) to provide the Real Time Correction link, you might also want to add a cut-down (6-way) piece of
[short header](https://www.adafruit.com/product/2940).

![Assembly_10](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Assembly_10.JPG)

### Lacquer the PCB

I recommend giving the PCB a coat of lacquer, especially if you are intending to use it on a UAV. Cover all of the surface mount components with
[Acrylic Protective Lacquer (conformal coating)](https://uk.rs-online.com/web/p/conformal-coatings/3217324/) except the connectors and IO pads.

![Assembly_11](https://github.com/PaulZC/ZED-F9P_FeatherWing_USB/blob/master/img/Assembly_11.JPG)

## Licence

Please see [LICENSE.md](./LICENSE.md) for details.

Enjoy!

**_Paul_**