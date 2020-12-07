# Phantom-ROV-Heading-Depth-Umb.-Turns
Arduino sketch to digitize heading and depth from Phantom ROV stock sensors. Also calcs umbilical turns and leak detect status.

This sketch runs on an Arduino UNO R3, that lives inside the Phantom ROV's mux console. It takes the stock heading and depth readings from the Phantom ROV's sensor, located in the STBD end cap. These are output as 0-5V signals from the 10-pin Amphenol 'AUX' connector on the rear of the Phantom's power slab. A custom, 4-pin aviation cable style feeds this data into the mux console. 

The following steps are completed:

# Heading signal:

This signal is output from the Phantom as a variable width pulse. This is based on 0/5v logic. The width of this pulse is proportional to the heading value, the emprically derived constant for this is ~0.189 msec pulse duration per degree of heading. Occassionally, the Arduino will miss capturing a pulse and will output a heading of zero as a result. To control for this, 3 sequential readings are taken, fed into an array, and the Mode of this array is taken. This protect against errant flip/flops of 'zero' heading. A function is also implemented to deal with heading 'wrap-around' to ensure that headings between 359 and 0 degrees are dealt with appropriately

# Depth signal: 

This is an analog signal which is output from the Phantom's power slab AUX connector as a voltage between 0 and 1V DC. Eeach millivolt increment between zero and 1V corresponds to one foot of seawater depth. The Phantom's 'stock' leak detector system is also coded into the depth signal -- if a leak is detected, a 24V DC circuit is shorted at the subsea end and this is conveyed through the Phantom's depth signal at surface. This will manifest as a fluctuation of +/- 75 mV in the depth signal. If the depth at the time that the leak detector circuit is trigger is less than 75, this WILL generate a DC signal with a reverse polarity. The handling of this special circumstance is dealt with through a 'double' voltage divider.

First, the 3.3V DC output form the Arduino is reduced to 1.05V through means of a 4.7k/10K ohm voltage divider. This result 1.05V is then fed into the high side of the second voltage divider. The other side of the the second voltage divider is the depth signal -- the second voltage divider pulls up the signal value by +80 to +90 mV, ensuring that the leak detector signal will always generate a value that can be digitized by the Arduino (i.e. it will not be less than 0 mV).

# Umbilical Turns:

This is simple, integer counter that increments any time the heading signal rolls over from 359 to 0, and decrements when it rolls from 0 to 359. If the mux console and power slab are turned on or off a different times, this can manifest as a 'fake' umbilical turn. To control for this, the umbilical turns value is locked at zero until the vehicle reaches 5 m of depth. 
