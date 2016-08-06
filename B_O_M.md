#/* BOM for O+C (rev 2d, 2e) */

- if your board is **red** and labelled **rev.2e** (on top of the board, underneath the 'ornament and crime' label), that's rev 2e. previous versions differ only slightly. see the [build guide](https://github.com/mxmxmx/O_C/wiki/build-it#bill-of-materials-bom) for details.

- the footprint for passives is **0805 throughout**. using 0603 everywhere will work just as fine, of course.

- versions prior to 2e: there's five 3-pin pads for trimpots (input/output calibration): **simply omit the trimpots**; the calibration can be done entirely in software. use five **jumper wires** instead and, ideally, **0.1% resistors** for the output stage (the "wiki" has more details): it's cheaper and makes calibration easier.


### SMD resistors (0805):
| value | #| part | note |
| ---: | ---: | --- | --- |
| 100R | 4x | e.g. mouser # 279-1623096-1 | 1% |
| 220R | 4x | e.g. mouser # 279-CPF0805B220RE1 | 1% |
| 510R | 2x | e.g. mouser # 603-RC0805JR-07510RL | 1-5% |
| 2k   | 1x | e.g. mouser # 660-RK73H2ATTE2001F | 1% |
| 10k  | 1x | e.g. mouser # 660-RK73H2ATTD1002F | 1-5% |
| **24k9**  | 4x | e.g. mouser # 756-PCF0805R-24K9BT1 | **0.1%** |
| 33k | 8x | e.g. mouser # 660-RK73H2ATTD3302F | 1% |
| 47k | 2x | e.g. mouser # 660-RK73H2ATTD4702F | 1% |
| **75k** | 4x | e.g. mouser # 660-RK73H2ATTD7502F | 1% (silkscreen says 49k9: ignore) |
| **100k** | 4x | e.g. mouser # 279-CPF0805B100KE | **0.1%** |
| 100k | 8x | e.g. mouser # 660-RK73H2ATTD1003F| 1% (or simply get 12 x 0.1%) |

### SMD caps (0805) (25V or better):
| value | #| type | note |
| --- | ---: | :---: | --- |
| 18p-22p | 4x | **C0G/NP0** (!) | e.g. mouser # 77-VJ0805A220GXAPBC |
| 560p | 4x | C0G/NP0 | e.g. mouser # 77-VJ0805A561JXAAC |
| 100n  | 12x | ceramic | e.g. mouser # 80-C0805C104K5R |
| 470n  | 3x  | ceramic | e.g. mouser # 77-VJ0805Y474JXJTBC |
| 1u    | 2x | ceramic | e.g. mouser # 581-08055C105K4Z2A |
| 10u   | 4x | ceramic (or tantal) | e.g. mouser # 81-GRM21BR6YA106KE3L |

## ICs/semis:

| what | # | package | part |
| --- | --- | ---: | --- |
| MCP6004 | 1x | (SOIC-14) | mouser # 579-MCP6004T-I/SL |
| OPA2172 | 2x | (SOIC-8) | mouser # 595-OPA2172IDR, and see note (†) below |
| DAC8565 |  1x | (TSSOP-16) | mouser # 595-DAC8565IAPW, 595-DAC8565ICPW |
| MMBT3904 (NPN) | 4x | (SOT-23) |  mouser # 512-MMBT3904 |
| 1N5817 (diode) | 2x | (DO-41) | e.g. mouser # 621-1N5817 |
| LM4040 5v0 | 1x | (SOT-23) | e.g. mouser # 926-LM4040DIM350NOPB |
| ADP150-3v3 | 1x |(TSOT) |  mouser # 584-ADP150AUJZ-3.3R7 |
| LM1117-50 | 1x | (SOT-223) |  mouser # 511-LD1117S50 |

- (†) something fancier (= more suitable, $$) than TL072 should be used for the DAC output stage; using TL072 will be ok, but ideally, you want something with (very) **low offset/noise/drift**. for example: OPA2172, OPA2277 

## misc through-hole:

| what | # | note | part | 
| --- | ---: | --- | --- | 
| 22uF cap | 2x | RM2.5 (35V or better) | e.g. mouser # 647-UPM1V220MDD1TD | 
| inductor | 1x | 10uH | e.g. mouser # 542-78F100-RC | 
| jacks |  12x | 'thonkiconn' (or kobiconn) | [PJ301M-12](https://www.thonk.co.uk/shop/3-5mm-jacks/) |
| encoders | 2x  | 24 steps w/ switch | e.g. PEC11R-4215K-S0024 (†) | 
| 2x5 pin header | 1x | 2.54mm (euro power connector) | e.g. mouser # 649-67996-410HLF | 
| 1x7 (1x8) OLED socket | 1x | 2.54mm, **low profile** ! | e.g mouser # 517-929870-01-08-RA | 
| 1x14 socket | 2x | 2.54mm, socket for teensy 3.x | see note (††) | 
| 1x14 pin header (to match) |  2x | 2.54mm, header for teensy 3.x | - |
| tact switches | 2x | multimecs 5E/5G | mouser #: 642-5GTH935 (†††) | 
| + caps | 2x | multimecs 1SS09-15.0 or -16.0 | mouser #: 642-1SS09-15.0, or -16.0 | 
| 3mm spacer | 1x | ∅ 3M, 10mm | - | 

- (†)  rotary encoder w/ switch: for instance: mouser # 652-PEC11R-4215F-S24 (15 mm, 'D' shaft); 652-PEC11R-4215K-S24 (15 mm shaft, knurled); 652-PEC11R-4220F-S24 (20 mm, 'D'), 652-PEC11R-4220K-S24 (20 mm, knurled), etc).
- (††) best to **use "machined pin" ones** (also called "round" or "precision"): the pcb holes are small.
- (†††) much cheaper is [sos](http://www.soselectronic.com/?str=371&artnum=102165&name=mec-5gth935).



## MCU/display:

| what | # | note | source |
| --- | --- | ---: | --- |
| teensy 3.2 / 3.1| 1x | **cut the V_usb/power trace!** | [oshpark](http://store.oshpark.com/products/teensy-3-1) / mouser # 485-2756 |
| OLED | 1x | SH1106 or SSD1306 / 1.3" / 128x64 | see note (†) | 

- (†) you can find these 1.3" displays on ebay or aliexpress for < 10$. as long as the description claims that they are `SH1106` or `SSD1306` and the pinout is: `GND - VCC - D0 - D1 - RST - DC - CS`, they should work (or `GND - VCC - CLK - MOSI - RES - DC - CS`, which is the same). **make sure you get the right size**: 1.3" (not 0.96")! 
- alternatively, the hardware/gerbers folder has **.brd/.sch** files for a/the OLED carrier board. in that case, you'd need to get the bare OLED. [for example here](http://www.buydisplay.com/default/serial-spi-1-3-inch-128x64-oled-display-module-ssd1306-white-on-black) (though there's cheaper options for getting bare OLEDs).

