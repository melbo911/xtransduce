# xtransduce

Arduino sketch to link X-Plane to a transducer. It's used to "feel" the rotor vibrations
based on its RPMs and gives feedback on G-forces and warns at VRS situations.
The volume is controlled by 5 digital outputs which are connected to a R2R resistor array
which is used as voltage dividers. So we have 31 different volumes/levels for the pulses, 
plus the option to mute it completely. 

The resistors have to be selected depending of the desired output levels. In my setup I was
using 56k for for R1 - R5 , 10k for R6 and 28k ( 2x56k parallel ) for R7 - R10. 

melbo @x-plane.org - 20210427


	#### Requires the XPLDirect plugin and library
	patreon:  www.patreon.com/curiosityworkshop
	YouTube:  https://youtube.com/channel/UCISdHdJIundC-OSVAEPzQIQ
	Plgn/Lib: https://www.patreon.com/posts/x-plane-direct-43341946


The output line (3.5mm jack) is supposed to feed a subwoofer amp which drives the transducer


#### TRANSDUCER
https://www.amazon.de/Bassshaker-Körperschallwandler-Heimkino-Playseats-Reckhorn/dp/B00AMH17GC/

#### MONO SUBWOOFER AMP
https://www.amazon.de/Nobsound-Subwoofer-Frequency-Channel-Amplifier-Black/dp/B0753CPVHS

