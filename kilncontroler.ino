/******************************************************************* 
 This Code liberally uses examples found online and as such is
 entirely open. I'm sticking with Limor's BSD license.
 Phil Showers
 
 ******************************************************************* 
  This is an example for the Adafruit Thermocouple Sensor w/MAX31855K

  Designed specifically to work with the Adafruit Thermocouple Sensor
  ----> https://www.adafruit.com/products/269

  These displays use SPI to communicate, 3 pins are required to  
  interface
  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 
 *******************************************************************  
  The PID code is from http://playground.arduino.cc/Code/PIDLibrary
 ******************************************************************* 
  The SD Card stuff is from the SD card example in the arduino 
  example library
 ******************************************************************* 
  The double to string code is from
  http://lordvon64.blogspot.com/2012/01/simple-arduino-double-to-string.html
 *******************************************************************/

#include "Adafruit_MAX31855.h"
#include "KilnRun.h" 
#include <LiquidCrystal.h>
#include <PID_v1.h>
#include <SPI.h>
#include <SD.h>
#include <Time.h>
#include <TimeAlarms.h>


const int CANDLE = 13; // candle (raise kiln to 200F and hold it there for 2, 4, 6, 8, or 12 hours before firing - cooks out all the water
const int CONE = 12; // choose kiln temp
const int HOLD = 11; // hold time at target temp, in hrs
const int SPEED = 10; // choose the speed/mode program 
const int START= 9; // starts the kiln heating program, CONE, HOLD, and SPEED are no longer read after starting, CLEAR will reset this.
const int RELAY = 6; // the pin for turning on and off the heating coils
const int DO = 3; // (data out) is an output from the MAX31855 (input to the microcontroller) which carries each bit of data
const int CLK = 5; // (clock) is an input to the MAX31855 (output from microcontroller) which indicates when to present another bit of data
const int CS = 4; // (chip select) is an input to the MAX31855 (output from the microcontroller) which tells the chip when its time to read the thermocouple and output more data.
const int LED = 3; // output LED
const int CLEAR = 2; // pushing this resets things
const int chipSelect = 10;
//
KilnRun thisRun;

// Initialize the Thermocouple
Adafruit_MAX31855 thermocouple(CLK, CS, DO);
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

//Define PID Variables we'll be connecting to
double Setpoint, Input, Output;
int WindowSize = 5000; 
unsigned long windowStartTime;

//Specify the links and initial tuning parameters
PID myPID(&Input, &Output, &Setpoint,2,5,1, DIRECT);

//These are internal variables to the parseButtons
int lastCANDLE = HIGH;
int lastCONE = HIGH;
int lastHOLD = HIGH;
int lastSPEED = HIGH;
int lastSTART = HIGH;
int lastCLEAR = HIGH;

bool sdCardInited = false;

/*******************************
 * SETUP
 ********************************/
void setup() {
  Serial.begin(9600);
  
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  else
    sdCardInited = true;
  Serial.println("card initialized.");
  
  /********** BUTTON SETUP **********/
  pinMode(CANDLE, INPUT); // initialize the button pin as a input
  digitalWrite(CANDLE, HIGH);       // turn on pullup resistor, *pull-up configuration — when the button is not pressed, the Arduino will sense high voltage. 
  pinMode(CONE, INPUT); // initialize the button pin as a input
  digitalWrite(CONE, HIGH);       // turn on pullup resistor
  pinMode(HOLD, INPUT); // initialize the button pin as a input
  digitalWrite(HOLD, HIGH);       // turn on pullup resistor
  pinMode(SPEED, INPUT); // initialize the button pin as a input
  digitalWrite(SPEED, HIGH);       // turn on pullup resistor
  pinMode(START, INPUT); // initialize the button pin as a input
  digitalWrite(START, HIGH);       // turn on pullup resistor
  pinMode(CLEAR, INPUT); // initialize the button pin as a input
  digitalWrite(CLEAR, HIGH);       // turn on pullup resistor
  // initialize the LED as an output
  pinMode(LED, OUTPUT);
  pinMode(RELAY, OUTPUT); // initialize the relay pin as output
  
  /********** PID Setup **********/
  //initialize the variables we're linked to
  Input = analogRead(0);
  windowStartTime = millis();
  Setpoint = 100;
  //tell the PID to range between 0 and the full window size
  myPID.SetOutputLimits(0, WindowSize);
  //turn the PID on
  myPID.SetMode(AUTOMATIC);
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  lcd.print("MAX31855 test");
  // wait for MAX chip to stabilize
  delay(500);
}// setup

void writeToSD(String msg)
{
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("kiln_log.txt", FILE_WRITE);
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(millis() + "," + msg);
    dataFile.close();
    // print to the serial port too:
    Serial.println(msg);
  }  
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  } 
}// writeToSD

/********************************
 * MAIN LOOP
 ********************************/
void loop()
{
  if(!sdCardInited)
    return;
  
   /********** THERMOCOUPLE LOOP **********/
   double temperature = thermocouple.readFarenheit();
   writeToSD("loop,temp:,"+doubleToString(temperature,1));
   
   /********** LCD LOOP **********/
   // basic readout test, just print the current temp
   lcd.setCursor(0, 0);
   lcd.print("Int. Temp = ");
   lcd.println(thermocouple.readInternalF());
   lcd.print("  "); 
   lcd.setCursor(0, 1);
   if (isnan(temperature)) 
   {
     lcd.print("T/C Problem");
   } 
   else 
   {
     lcd.print("F = "); 
     lcd.print(temperature);
     lcd.print("  "); 
   }   
   
   /********** INPUT BUTTON LOOP **********/
   parseButtons();
   
   /********** PID LOOP **********/
   if(thisRun.isStarted())
     kiln(temperature);
     
   /********** DELAY 30 SEC **********/
   delay(30000);              // wait for 30 seconds
}// loop




/********************************
 * Called from MAIN LOOP
 ********************************/
 
 /**
  * This fn handles the task on turning on and off the kiln relay
  * TODO: add kiln.getTargetTemp(currentTemperature); somewhere...
  */
 void kiln(double currentTemperature)
 {
   Input = currentTemperature;
   myPID.Compute();

  /************************************************
   * turn the output pin on/off based on pid output
   ************************************************/
  if(millis() - windowStartTime>WindowSize)
  { //time to shift the Relay Window
    windowStartTime += WindowSize;
  }
  if(Output < millis() - windowStartTime) digitalWrite(RELAY,HIGH);
  else digitalWrite(RELAY,LOW);

  /*****************
  Setpoint = temperature;
  Input = temperature;
  Compute();
  analogWrite(3,Output);
  digitalWrite(relay, HIGH);   	// turn the relay on
  //  */
 }// kiln
 
 
int parseButtons()
{
   int buttonState = 0;         // current state of the button  
  
   buttonState = digitalRead(CANDLE); // read the pushbutton input pin   
   //   *pull-up configuration — when the button is not pressed, the Arduino will sense high voltage.
   if (buttonState != lastCANDLE) // compare the buttonState to its previous state
   {
     if (buttonState == HIGH) // if the state has changed, and the state is HIGH meaning the button is not pressed, perform the button's ev0ent
     {
       thisRun.candlePressed();
     }// button released
     else
     {
       //button state is LOW, meaning that the button is currently being pressed, when it is released the change will trigger the event
     }
   }//button state changed
   lastCANDLE = buttonState;// save the current state as the last state
   
   buttonState = digitalRead(CONE); // read the pushbutton input pin   
   //   *pull-up configuration — when the button is not pressed, the Arduino will sense high voltage.
   if (buttonState != lastCONE) // compare the buttonState to its previous state
   {
     if (buttonState == HIGH) // if the state has changed, and the state is HIGH meaning the button is not pressed, perform the button's event
     {
       thisRun.conePressed();
     }// button released
     else
     {
       //button state is LOW, meaning that the button is currently being pressed, when it is released the change will trigger the event
     }
   }//button state changed
   lastCONE = buttonState;// save the current state as the last state
   
   buttonState = digitalRead(HOLD); // read the pushbutton input pin   
   //   *pull-up configuration — when the button is not pressed, the Arduino will sense high voltage.
   if (buttonState != lastHOLD) // compare the buttonState to its previous state
   {
     if (buttonState == HIGH) // if the state has changed, and the state is HIGH meaning the button is not pressed, perform the button's event
     {
       thisRun.holdPressed();
     }// button released
     else
     {
       //button state is LOW, meaning that the button is currently being pressed, when it is released the change will trigger the event
     }
   }//button state changed
   lastHOLD = buttonState;// save the current state as the last state
   
   buttonState = digitalRead(SPEED); // read the pushbutton input pin   
   //   *pull-up configuration — when the button is not pressed, the Arduino will sense high voltage.
   if (buttonState != lastSPEED) // compare the buttonState to its previous state
   {
     if (buttonState == HIGH) // if the state has changed, and the state is HIGH meaning the button is not pressed, perform the button's event
     {
       thisRun.speedPressed();
     }// button released
     else
     {
       //button state is LOW, meaning that the button is currently being pressed, when it is released the change will trigger the event
     }
   }//button state changed
   lastSPEED = buttonState;// save the current state as the last state
   
   buttonState = digitalRead(START); // read the pushbutton input pin   
   //   *pull-up configuration — when the button is not pressed, the Arduino will sense high voltage.
   if (buttonState != lastSTART) // compare the buttonState to its previous state
   {
     if (buttonState == HIGH) // if the state has changed, and the state is HIGH meaning the button is not pressed, perform the button's event
     {
       thisRun.startPressed();
     }// button released
     else
     {
       //button state is LOW, meaning that the button is currently being pressed, when it is released the change will trigger the event
     }
   }//button state changed
   lastSTART = buttonState;// save the current state as the last state
   
   buttonState = digitalRead(CLEAR); // read the pushbutton input pin   
   //   *pull-up configuration — when the button is not pressed, the Arduino will sense high voltage.
   if (buttonState != lastCLEAR) // compare the buttonState to its previous state
   {
     if (buttonState == HIGH) // if the state has changed, and the state is HIGH meaning the button is not pressed, perform the button's event
     {
       thisRun.clearPressed();
     }// button released
     else
     {
       //button state is LOW, meaning that the button is currently being pressed, when it is released the change will trigger the event
     }
   }//button state changed
   lastCLEAR = buttonState;// save the current state as the last state
  return buttonState;
}// parseButton

//Rounds down (via intermediary integer conversion truncation)
String doubleToString(double input,int decimalPlaces){
  if(decimalPlaces!=0){
    String string = String((int)(input*pow(10,decimalPlaces)));
    if(abs(input)<1){
      if(input>0)
        string = "0"+string;
      else if(input<0)
        string = string.substring(0,1)+"0"+string.substring(1);
    }
    return string.substring(0,string.length()-decimalPlaces)+"."+string.substring(string.length()-decimalPlaces);
  }
  else
  {
    return String((int)input);
  }
}


