/*
 * Arduino sketch to link X-Plane to a transducer. It's used to "feel" the rotor vibrations
 * based on its RPMs and gives feedback on G-forces and warns at VRS situations. 
 * The volume is controlled by 5 digital outputs which are connected to diodes and resistors
 * which are used as voltage dividers. So we have 5 different volumes/levels for the pulses plus the
 * option to mute it completely.
 *
 *  requires XPLDirect plugin and library
 *  (free from here https://www.patreon.com/curiosityworkshop)
 * 
 * melbo @x-plane.org - 20210419
 *
 */


#include <arduino.h>
#include <XPLDirect.h>              // include file for the X-plane direct interface
XPLDirect Xinterface(&Serial);      // create an instance of it

// constants

// global variables
int volume[] = {5,6,7,8,9};         // digital pins for 5 different volume levels

long int counter;
bool state;
float val;
int vol;
int lastVol;
long int v_z;
long int v_x;
float force;
float airSpeed;
float verticalSpeed;
float rpm[8];
float blades[8];

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

   Xinterface.registerDataRef(F("sim/cockpit2/engine/indicators/prop_speed_rpm"), XPL_READ, 100, 0, rpm,0);  
   Xinterface.registerDataRef(F("sim/aircraft/prop/acf_num_blades"), XPL_READ, 100, 1, blades,0);    
   Xinterface.registerDataRef(F("sim/multiplayer/position/plane9_x"), XPL_WRITE, 100, 0, &v_x);
   //Xinterface.registerDataRef(F("sim/multiplayer/position/plane9_z"), XPL_READWRITE, 100, 0, &v_z);
   Xinterface.registerDataRef(F("sim/flightmodel/forces/g_nrml"), XPL_READ, 100, 0, &force);
   Xinterface.registerDataRef(F("sim/flightmodel/position/indicated_airspeed"), XPL_READ, 100, 1, &airSpeed);
   Xinterface.registerDataRef(F("sim/cockpit2/gauges/indicators/vvi_fpm_pilot"), XPL_READ, 100, 1, &verticalSpeed);


   digitalWrite(LED_BUILTIN, LOW);

   // init output ports and put them quiet
   for (int i=0; i<sizeof(volume);i++){
      pinMode(volume[i],OUTPUT);
      digitalWrite(volume[i],0);
   }

   // init vars
   rpm[0]        = 0.0;
   blades[0]     = 0.0;
   val           = 1.0;
   airSpeed      = 0.0;
   verticalSpeed = 0.0;
   vol           = 2;
   lastVol       = 0;
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
  
   Xinterface.xloop();  //  needs to run every cycle

   if ( m >= counter ) {
      val = (rpm[0] * blades[0] / 60);
 
      if ( val > 0.0 ) {         // avoid div/0

         // set volume based on rotor rpm
         if ( val < 5.0 ) {      // low rpm = lowest volume
            vol = 1;
         } else {
            if ( val < 10.0 ) {  // a bit louder during startup
               vol = 2;
            } else {
               vol = 3;          // normal volume
            }
         }

         // adjust volume based on G-force
         if ( force > 1.2 ) {     
            if ( force > 1.4 ) {  
               vol = 5;          // g-force above 1.4 = max volume
            } else {
               vol = 4;          // g-force above 1.2 = louder than normal
            }
         }

         // VRS alert at less than 30kt with decend of more than 300 ft/min
         if ( airSpeed < 30.0 && verticalSpeed < -300.0 ) {  
            vol = 5;             // VRS warning
         }

         // provide feedback to dataref
         v_x = int(rpm[0]);

         counter = m + int(1000/val) ;   // book next pit-stop
     
         setVol(vol,state);      // set volume and state
      } else {

         val = 0;
         v_x = 999;              // rpm 0 or undef
         counter = m + 500;      // restart counter
         setVol(vol,0);          // switch off output
      }
     
      state=!state;             // toggle state
      digitalWrite(LED_BUILTIN, state);  // LED shows we're still alive
   }
  
   delay(10);   // relief cpu and reduce loop count
}

void setVol(int vol, int val) {
   if ( vol != lastVol ) {
      if ( lastVol > 0 ){
         digitalWrite(volume[lastVol-1],0);
      }
      lastVol = vol;
   }
   if ( vol > 0 && vol <= sizeof(volume) ) {
      digitalWrite(volume[vol-1],val);
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
