#/* BOM for O+C */


**SMD resistors (0805):**

- 200R :         4x 
- 510R :         6x 
- 9k1 :          4x
- 10k :          12x (ideally, matched in pairs) 
- 33k :          8x  
- 49k9:          4x
- 100k :         20x

**SMD caps (0805):**

- 22pf  : 4x (NP0)
- 100nF : 15x  
- 330nF : 2x 
- 1uF   : 1x (may be 1206)

**ICs/semis:**

- TL074* (SOIC) : 3x  
- DAC8565 (TSSOP): 1x
- NPN transistors (MMBT3904, SOT-23) 4x
- 1N5817 (diode, DO-41): 2x
- BAT54S (Schottky (dual/series) SOT-23): 4x
- LM4040-2.5 (SOT-23) : 1x (optional**)

**misc *through-hole*:**

- 22uF  (electrolytic) : 2x
- 100nf (film/ceramic, RM2.5) : 1x
- 100nF (film/ceramic, RM5)   : 1x
- 150nF (film, RM5)   : 1x
- 10uH inductor : 1x 
- 7805   5v regulator: 1x (TO 220)
- 79L05 -5v regulator: 1x (TO 92) (or use (or LM4040-5 + 470R (0805))) (***)
- 78L33 3v3 regulator: 1x (TO 92)
- **miniature** (!) switch (pcb mount); on/off (SPDT) : 1x **OR** if to switch bipolar/unipolar output by jumper: 1x3 pin header (2.54mm) + jumper) **OR** if you want a different range/the option to adjust the offset properly: 50k trimpot (cermet type) -- see the 'wiki' for details
- trimpot 100k (inline / 9.5mm): 1x
- trimpot 2k   (inline / 9.5mm): 4x
- jacks: 'thonkiconn' (or kobiconn): 12x
- encoders (24 steps (ish)) with switch (e.g. PEC11R-4215K-S0024) : 2x (****)
- 2x5 pin header, 2.54mm : 1x (euro power connector)
- 1x7 pin header, 2.54mm : 1x (for oled)
- 1x7 socket, 2.54mm (low profile) : 1x (for oled)
- 1x14 sockets + matching pin headers : 2x (for teensy 3.1; best to use "machine pin" ones, the pcb holes are smallish)
- tact switches (multimecs 5E/5G): 2x (mouser #: 642-5GTH935)
- + caps (multimecs 1SS09-15.0 or -16.0): 2x (mouser #: 642-1SS09-15.0, or -16.0)
- teensy3.x : 1x **(cut the V_usb/power trace)**
- oled: i've used these: http://www.ebay.com/itm/New-Blue-1-3-IIC-I2C-Serial-128-x-64-OLED-LCD-LED-Display-Module-for-Arduino-/261465635352  [ the pinout should be: GND - VCC - D0 - D1 - RST - DC - CS  : adafruit's 128x64 (http://www.adafruit.com/products/326) should work as well (in terms of the hardware), at least the corresponding pads are on the pcb as well (untested).


##Notes:


(*) something fancier (suitable, $$) could be used for the DAC outputs, of course. 2x quad op amps are needed (2 per channel; gain is twice 2x). i've tried, and wasn't sure the difference that it might make is all that relevant.

(**) the lm4040 is not really needed, but if installed this requires adjusting some values to get the ADC range approx. right (2v5 rather than 3v3); specifically, say 4x 49k9 - > 4x 62k; and 4x 100k -> 4x 130k (or 120k)
 
(***) as above: the LM4040 is not really needed in this application, but it'll improve ADC performance.

(****)  rotary encoder w/ switch: for instance: mouser # 652-PEC11R-4215F-S24 (15 mm, 'D' shaft); 652-PEC11R-4215K-S24 (15 mm shaft, knurled); 652-PEC11R-4220F-S24 (20 mm, 'D'), 652-PEC11R-4220K-S24 (20 mm, knurled), etc)

