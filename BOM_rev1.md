#/* BOM for O+C */


the footprint for passives is 0805 but 0603 would work just as fine, of course.

**SMD resistors (0805):**

- 100R :         4x 
- 220R : 		 4x
- 510R :         2x 
- 2k :           1x
- 24k : 		 4x	
- 33k :          8x
- 47k :			 2x (that's what the silkscreen says -- you can use 49k9 instead)
- 49k9:          4x
- 100k :         24x

**SMD caps (0805):**

- 22p   : 4x (C0G/NP0)
- 100n  : 1x
- 100nF : 15x  
- 330nF : 2x 
- 1uF   : 2x (ceramic or tantal)
- 10uF  : 3x (ceramic or tantal)

**ICs/semis:**

- TL074 (SOIC) : 1x  
- TL072 or (superior) equivalent (SOIC), eg OPA2172: 2x (†)
- DAC8565 (TSSOP): 1x
- NPN transistors (MMBT3904, SOT-23) 4x
- 1N5817 (diode, DO-41): 2x
- BAT54S (Schottky (dual/series) SOT-23): 4x
- LM4040-5.0 (SOT-23) : 1x
- ADP150-3v3 (3v3 regulator, TSOT): 1x (mouser # 584-ADP150AUJZ-3.3R7)
- LM1117-50 (5v0 LDO reg., SOT-223): 1x
- ***LM4040-2.5 (SOT-23) : 1x*** (optional (††))

**misc *through-hole*:**

- 100nF (film/ceramic, RM5)   : 1x
- 470nF (film, RM5)   : 1x
- 22uF  (electrolytic) : 2x
- 10uH inductor : 1x 
- 78L33 3v3 regulator: 1x (TO 92)

- trimpot 2k   (inline / 9.5mm) : 4x (DAC output trim)
- trimpot 100k (inline / 9.5mm) : 1x (ADC/CV offset trim)

- jacks: 'thonkiconn' (or kobiconn): 12x
- encoders (24 steps (ish)) with switch (e.g. PEC11R-4215K-S0024) : 2x (†††)
- 2x5 pin header, 2.54mm : 1x (euro power connector)
- 1x7 socket, 2.54mm **(low profile)** : 1x (for oled)
- 1x14 socket, 2.54mm : 2x (for teensy 3.1): **use "machined pin" ones (also called "round" or "precision"), the pcb holes are small**)
- 1x14 pin header to match, 2.54mm : 2x (for teensy 3.1): 
- tact switches (multimecs 5E/5G): 2x (mouser #: 642-5GTH935) 
- + caps (multimecs 1SS09-15.0 or -16.0): 2x (mouser #: 642-1SS09-15.0, or -16.0)
- teensy 3.x : 1x **(cut the V_usb/power trace)**
- oled: SSD1106 128x64 : 1x (††††)


##Notes:


(†) something fancier (suitable, $$) than TL072 could/should be used for the DAC output stage; in my experience, TL072 will work just fine but ideally, you want something will (very) low offset/noise/drift. for example: OPA2172, OPA2277 

(††) the lm4040-2.5 (for teensy AREF) is not really needed, but if installed this requires adjusting some values to get the ADC range approx. right (ie 2v5 rather than 3v3); specifically, say, 4 x 130k (not 100k) for the CV input resistors (more info in the 'wiki' -- we're dealing with the standard inverting amplifier, with 33k feedback)

(†††)  rotary encoder w/ switch: for instance: mouser # 652-PEC11R-4215F-S24 (15 mm, 'D' shaft); 652-PEC11R-4215K-S24 (15 mm shaft, knurled); 652-PEC11R-4220F-S24 (20 mm, 'D'), 652-PEC11R-4220K-S24 (20 mm, knurled), etc)

(††††) i've used these: http://www.ebay.com/itm/New-Blue-1-3-IIC-I2C-Serial-128-x-64-OLED-LCD-LED-Display-Module-for-Arduino-/261465635352 . others might work, too, as long as they're SSD1106 or SSD1306 and the pinout is: GND - VCC - D0 - D1 - RST - DC - CS  

