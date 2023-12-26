I2S to Toslink PCB

Kicad 7.0 schematic and PCB project which purpose is to create a PCB that
can generate optical AES/EBU signal for Toslink audio adapter. PCB uses the
WM8805 chipset as a codec for transforming the signal from I2S to AES/EBU Toslink.

PCB has following input pins:
- 5V power
- GND
- I2S data
- I2S LRCLK
- I2S BCLK
- I2S MCLK (optional)

PCB can either be configured to use the external MCLK or generate the MCLK from the BCLK.
Generation of MCLK is usually needed because the WM8805 does not always work with the external
MCLK and that is hearable as a small errors and cracks on audio.

Project will also need the firmware for ATmega chip so that it can configure the PLL generator
and WM8805 over I2C lines to right mode.
