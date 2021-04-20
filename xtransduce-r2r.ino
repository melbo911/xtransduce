/*
 * Arduino sketch to link X-Plane to a transducer. It's used to "feel" the rotor vibrations
 * based on its RPMs and gives feedback on G-forces and warns at VRS situations. 
 * The volume is controlled by 5 digital outputs which are connected to diodes and resistors
 * which are used as voltage dividers. So we have 5 different volumes/levels for the pulses plus the
 * option to mute it completely.
 * 
 * melbo @x-plane.org - 20210420
 *
 */


#include <arduino.h>
#include <XPLDirect.h>              // include file for the X-plane direct interface
XPLDirect Xinterface(&Serial);      // create an instance of it

/*
	port B    8 - 13   ( 14 + 15 = crystal pins. dont use )

	port C    A0 - A5  ( analog inputs )

	port D    0 - 7    ( 0 = RX , 1 = TX . dont use )
*/

// constants

// rpm * blades
const PROGMEM int bladeLevel[] = {0, 0, 2x350, 3x350, 4x350, 5x350, 6x360};

long int counter;
bool state;
float val;
int vol;
int alive;
int watchdog;
long int v_x;
long int v_y;
long int v_z;
float zulu;
float zuluLast;
float force;
float airSpeed;
float verticalSpeed;
float rpm;
float blades;

/*
 *  SETUP
 *  
 *  the setup function runs once when you press reset or power the board
 *
 */
void setup() { 
   pinMode(LED_BUILTIN, OUTPUT);     // built in LED on arduino board for debug and demonstration purposes
   
   Serial.begin(XPLDIRECT_BAUDRATE); // start serial interface.
   Xinterface.begin("Xtransduce");   // Send a texual identifier of your device as a parameter.
 
   ErrorBlink(LED_BUILTIN, 5);       // let everyone know we are awake
   
   //Initialize serial and wait for port to open:
   while (!Serial) {
      ErrorBlink(LED_BUILTIN, 2);
      delay(300);
   }

   Xinterface.registerDataRef(F("sim/cockpit2/engine/indicators/prop_speed_rpm"), XPL_READ, 100, 1, &rpm,0);  
   Xinterface.registerDataRef(F("sim/aircraft/prop/acf_num_blades"), XPL_READ, 100, 1, &blades,0);    
   Xinterface.registerDataRef(F("sim/multiplayer/position/plane9_x"), XPL_WRITE, 100, 0, &v_x);
   Xinterface.registerDataRef(F("sim/multiplayer/position/plane9_y"), XPL_WRITE, 100, 0, &v_y);
   Xinterface.registerDataRef(F("sim/multiplayer/position/plane9_z"), XPL_READ, 100, 0, &v_z);
   Xinterface.registerDataRef(F("sim/flightmodel/forces/g_nrml"), XPL_READ, 100, .1, &force);
   Xinterface.registerDataRef(F("sim/time/zulu_time_sec"), XPL_READ, 100, 1, &zulu);
   Xinterface.registerDataRef(F("sim/flightmodel/position/indicated_airspeed"), XPL_READ, 100, 1, &airSpeed);
   Xinterface.registerDataRef(F("sim/cockpit2/gauges/indicators/vvi_fpm_pilot"), XPL_READ, 100, 1, &verticalSpeed);

   digitalWrite(LED_BUILTIN, LOW);

   // init output ports and put them quiet
   DDRB  &= B11100000;  // set bit 0-4 (pins 8-12) as INPUT
   PORTB &= B11100000;  // set bit 0-4 (pins 8-12) to LOW ( no pulldown )

   // init vars
   rpm           = 0.0;
   blades        = 0.0;
   val           = 1.0;
   airSpeed      = 0.0;
   verticalSpeed = 0.0;
   force         = 0.0;
   zulu          = 0.0;
   zuluLast      = 0.0;
   watchdog      = 0;
   alive         = 0;
   vol           = 2;
   counter       = 0;
}


/* 
 * MAIN LOOP
 * 
 * the loop function runs over and over again forever
 * 
 */
void loop() {
   long int m = millis();
  
   Xinterface.xloop();            //  needs to run every cycle

   if ( watchdog < 0 ) {
      if ( zulu > zuluLast ) {    // sim still running ??
         zuluLast = zulu;
         alive = 1;
      } else {
         alive = 0;
      }
      watchdog = 500;
   } else {
      watchdog--;
   }
      
   if ( m >= counter ) {
      val = (rpm * blades / 60);
     
      if ( val > 0.01 && alive ) { // avoid div/0
         
         // set volume based on max rotor rpm
         vol = ( (rpm * blades) / (bladeLevel[int(blades)]/15) * 15) ;
         if ( vol > 15 ) { vol = 15; }

         // adjust volume based on G-force
         if ( force > 1.0 ) {     
            if ( force > 2.0 ) {  
               vol = 31;                // g-force above 2.0 = max volume
            } else {
               vol = int(vol * force);  // g-force above 1.0 = louder than normal
            }
         }

         // VRS alert at IAS less than 30kt with decend of more than 300 ft/min
         if (  (airSpeed < 30.0 && verticalSpeed < -300.0) ) {  
            vol = 31;              // VRS warning 
         }

         // provide feedback about rpms to DatarefTool
         v_x = int(val);
         v_y = int(vol);

         //vol = v_z;    // DEBUG !!!! get VOL input from Datareftool

         counter = m + int(1000/val) ;   // book next pit-stop
            
         if ( state ) {
            setVol(vol);          // set volume and HIGH state
         } else {
            setVol(0);            // set LOW state
         }
         
      } else {

         v_x = 999;               // rpm 0 or undef
         counter = m + 500;      // restart counter, see you in 500 ms
         setVol(0);               // switch off output
      }
     
      state=!state;               // toggle state
      digitalWrite(LED_BUILTIN, state);  // LED shows we're still alive
   }
  
   delay(2);                      // relief cpu and reduce loop count
}

void setVol(int vol) {

   DDRB   &= B11100000;           // set 0-4 as INPUT ( dont touch 6-7 )
   PORTB  &= B11100000;           // set 0-4 to LOW   ( no pullup )

   if ( vol > 0 ) {
      if ( vol > 31 ) {           // 5 bit only
         vol = 31;
      }
      DDRB  |= vol;               // only set "vol" to OUTPUT
      PORTB |= vol;               // only set "vol" to HIGH
   }
}

void ErrorBlink(int pin, int count) {
   for (int i = 0; i< count; i++) { 
      digitalWrite(pin, HIGH);
      delay(200);
      digitalWrite(pin, LOW);
      delay(100);
   }
}


//
// EOF
//
