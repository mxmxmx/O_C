#/* BOM for O+C (rev 2c) */

- if your board is is labelled rev.2c (on top of the board, underneath the 'ornament and crime' label), that's rev 2c. previous versions differ only slightly.

- the footprint for passives is **0805 throughout**, except for the four 10n caps (0603). using 0603 everywhere will work just as fine, of course.

- there's four 3-pin pads for 2k trimpots (output calibration): **we recommend omitting the trimpots**, and doing the calibration entirely in software. use four jumper wires instead and, ideally, 0.1% resistors for the output stage (the "wiki" has more details): it's cheaper and makes calibration easier (and no less accurate).


### SMD resistors (0805):
| value | #| note |
| --- | ---: | :---: |
| 100R | 4x | - |
| 220R | 4x | - |
| 510R | 2x | - |
| 2k   | 1x | - |
| 10k  | 1x | - |
| **24k9**  | 4x | **0.1%** (**if** building **with** trimpots, use 24k (1%) rather than 24k9 (0.1%))|
| 33k | 8x | - |
| 49k9 | 6x | silkscreen says 2x 47k, 4x 49k9: you can use 49k9 throughout |
| **100k** | 12x | **4 of which should be 0.1%** |



### trimpots (throughhole): 
| value | type | #| note |
| --- | --- | ---: | :---: |
| trimpot 100k | (inline / 9.5mm) | 1x | ADC/CV offset trim |
| **optional:** trimpot 2k | (inline / 9.5mm) | 4x | DAC output trim (**omit: use 0.1% resistors instead**) |

### SMD caps (0603) (16V) :
| value | #| type|
| --- | ---: | :---: |
| 10n | 6x | C0G/NP0 |

### SMD caps (0805) (25V or better):
| value | #| type|
| --- | ---: | :---: |
| 18p-22p | 4x | **C0G/NP0** (!) |
| 100n  | 12x | ceramic | 
| 470n  | 1x  | ceramic |
| 1u    | 1x | ceramic |
| 10u   | 4x | ceramic (or tantal) |

## ICs/semis:

| name | package| #| note |
| --- | --- | ---: | --- |
| MCP6004 | (SOIC-14) | 1x  | mouser # 579-MCP6004T-I/SL |
| OPA2172 | (SOIC-8) | 2x  | see note (†) below |
| DAC8565 | (TSSOP-16) | 1x | mouser # 595-DAC8565IAPW |
| MMBT3904 (NPN) | (SOT-23) | 4x | mouser # 512-MMBT3904 |
| 1N5817 (diode) | (DO-41) | 2x | reverse voltage protection |
| LM4040-5.0 | (SOT-23) | 1x | 5v0 reference |
| ADP150-3v3 | (TSOT) | 1x | mouser # 584-ADP150AUJZ-3.3R7 |
| LM1117-50 | (SOT-223) | 1x | mouser # 511-LD1117S50 |

## misc through-hole:

| what | note | part | # | 
| --- | :--- | --- | ---: | 
| 470nF cap | RM5, 16V+ | e.g. WIMA MKS2 | 1x |
| 22uF cap | electrolytic (35V or better) | - | 2x |
| inductor | 10uH | e.g. mouser # 542-78F100-RC | 1x |
| jacks | 'thonkiconn' (or kobiconn) | PJ301M-12 | 12x |
| encoders | 24 steps w/ switch | e.g. PEC11R-4215K-S0024 (†††) | 2x  |
| 2x5 pin header | 2.54mm (euro power connector) | - | 1x |
| 1x7 socket | 2.54mm (OLED socket) | **(low profile)** !! | 1x |
| 1x14 socket | 2.54mm, socket for teensy 3.x | see note (††††) | 2x | 
| 1x14 pin header (to match) | 2.54mm, header for teensy 3.x | - | 2x |
| tact switches | multimecs 5E/5G | mouser #: 642-5GTH935 | 2x |
| + caps | multimecs 1SS09-15.0 or -16.0 | mouser #: 642-1SS09-15.0, or -16.0 | 2x |

## MCU/display:

| name | note | # |
| --- | --- | ---: |
| teensy 3.2 / 3.1| **(cut the V_usb/power trace!)** | 1x |
| OLED | SH1106/SSD1306 1.3" 128x64 (†††) | 1x  |


## Notes:


(†) something fancier (more suitable, $$) than TL072 should be used for the DAC output stage; using TL072 will be ok, but ideally, you want something with (very) low offset/noise/drift. for example: OPA2172, OPA2277 

(†††)  rotary encoder w/ switch: for instance: mouser # 652-PEC11R-4215F-S24 (15 mm, 'D' shaft); 652-PEC11R-4215K-S24 (15 mm shaft, knurled); 652-PEC11R-4220F-S24 (20 mm, 'D'), 652-PEC11R-4220K-S24 (20 mm, knurled), etc)

(†††) you can find these 1.3" displays on ebay or aliexpress for < 10$. as long as the description claims that they are `SH1106` or `SSD1306` and the pinout is: `GND - VCC - D0 - D1 - RST - DC - CS`, they should work (or `GND - VCC - CLK - MOSI - RES - DC - CS`, which is the same). make sure you get the right size --  1.3" (not 0.96")! alternatively, the hardware/gerbers folder has .brd/.sch files for a/the OLED carrier board. in that case, you'd need to get the bare OLED. [for example here](http://www.buydisplay.com/default/serial-spi-1-3-inch-128x64-oled-display-module-ssd1306-white-on-black), though there's cheaper options for getting bare OLEDs.

(††††) best to **use "machined pin" ones (also called "round" or "precision"), the pcb holes are small**)
