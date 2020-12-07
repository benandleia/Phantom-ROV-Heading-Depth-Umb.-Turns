//Read in the 5V DC pulse from the Phantom Aux output port (Pin 1 = Common, Pin 4 = Heading Signal)
//The heading information is carried while the signal on Pin 4 of the Aux output cable is LOW. Measure this duration (msecs) and convert to
//heading information.

#include "AltSoftSerial.h"
#include "QuickStats.h"
#include "math.h"

AltSoftSerial altSerial;  //Create an additional serial port instance, using AltSoftSerial library
QuickStats Stats; //Create a stats object

const byte INPIN = 7; //Digital Pin number to read in Heading pulse length
int depthPin = 5; //Set Analog Pin A5 to read in the depth value. 
int beep_pin = 4; //Set digital PWM pin 9 to power the 5V computer speaker

int heading_initial = 0;
String header = "$ROV_HDG_DP,";
float leak_array[10];
float heading_array[3];
float depth_array[9];
//char leak_warning[] = "LEAK DETECTED";
//char normal[] = " ";
int leak_time = 0;
int round_depth;
int round_heading;
char warning[14];

void setup() {

//Initialize serial communications at 9600 baud. 

   Serial.begin(9600);
   pinMode(INPIN, INPUT);
   analogReference(INTERNAL); //Sets the upper end of the analogRead() function to 1.1 V, so full range of measure is 1.1/1024 or 1 mV per increment.
   altSerial.begin(9600);
}


void loop() {

//Read in the pulse lengths.

  unsigned long pulse_read_now;
  
  noInterrupts();
  pulse_read_now = pulseIn(INPIN, LOW, 100000UL);
  float pulse_current = pulse_read_now / 1000.0;  //Convert from micro seconds to msecs.
  int heading_current = pulse_current / 0.1811797; //Emprically derived constant, msecs/degree of heading. 
  interrupts();

//Calculate difference between initial heading at time of power on, and current heading. 

  int heading_difference = heading_current - heading_initial;
  int heading_correct = wrap360(heading_difference); //Contend with compass wrapping around 0

//Add heading readings to an array of size = 3, then take the mode of this array. This protects against errant 0 heading values.

  for(int i=2; i > -1; i--){
    if(i == 0){
    heading_array[i] = heading_correct;
    } else {
    heading_array[i] = heading_array[i -1];
    }
  }
  
//Take the mode of the heading array
   float heading_mode = Stats.mode(heading_array, 3, 0.00001); 

//Cast to int(), round it.
  round_heading = int(heading_mode + 0.5);
  

  

//Take 9 readings in succession, and then average them. Help to remove some of the variability in the signal. Check this array for larger fluctuations in depth; if they are detected, trigger the leak detector.

  for(int i=8; i > -1; i--){
    if(i == 0){
    depth_array[i] = analogRead(depthPin);
    } else {
    depth_array[i] = depth_array[i -1];
    }
  }

  float depth_avg = Stats.average(depth_array, 9);
  float meters = (depth_avg * 0.3584) - 42; //Scales 0-1023 A to D values to a depth in meters. Empirically derived values, based on passing small incremental voltages through benchtop power supply.

//Round the depth to nearest meter.

  round_depth = int(meters + 0.5);

  //To catch leak detection behaviour, store depth readings in an array of 9 values, continuously update this array. If wild fluctuations are
  //detected, change the message header to 'LEAK DETECTED'.
  float leak_val = Stats.stdev(depth_array, 9);


  //If the standard deviation of the leak value array exceeds 10, there is a large variation in the depths, and this should be reflected on the 
  //power slab's display as well. 
  
  if(leak_val < 10)  //If the leak value is active, and it has been for more than 5 seconds, trigger the leak sensor response.
  {
  leak_time = 0;
  strcpy(warning, " ");
  } else {
    leak_time = leak_time + 1; //Increment by approximately the number of milliseconds per progam loop.
    if(leak_time > 20) //If the leak value is still active, trigger the response.
    {
    strcpy(warning, "7777");
    tone(beep_pin, 600, 1000);
    }
  }
  
  //Compare current heading value to the one from previous iteration of the loop, find the difference.

  static int lastVal = heading_correct;
  lastVal = heading_correct - lastVal;
  static int turns_counter = 0;  //Set initial value of turns counter to 0.

  if(lastVal == 0)
  {}
  else if(lastVal < -90) //If difference between last and current value is highly negative, increment the counter. Choose a value of 90, since
  {                      //unlikely that the vehicle will turn more than 90 degrees/sec in water.
    turns_counter++ ;
  }
  else if(lastVal > 90) //If difference between last and current value is highly positive, decrement the counter
  {
    turns_counter-- ;
  }

  //To protect against errant umbilical turn flip/flops at surface, lock the umbilical turns to 0 when the depth is less than 10 m.

  if(meters < 5)
  {
    turns_counter = 0;
  }

//Print the message

  altSerial.print(header);
  altSerial.print(round_heading);
  altSerial.print(",");
  altSerial.print(round_depth);
  altSerial.print(",");
  altSerial.print(turns_counter);
  altSerial.print(",");
  altSerial.print(warning);
  altSerial.print(",");
  altSerial.print(leak_val);
  altSerial.println();
  
  lastVal = heading_correct;

delay(150);
}

//This function re-maps compass values that wrap around 0, so that an accurate distance between the initial compass readings and those taken 
//a dive can always be calculated correctly.

int wrap360(int direction)
{
  while(direction > 359) direction -= 360;
  while(direction < 0) direction += 360;
  return direction;
}
