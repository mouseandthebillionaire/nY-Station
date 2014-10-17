/*
 * Firmata is a generic protocol for communicating with microcontrollers
 * from software on a host computer. It is intended to work with
 * any host computer software package.
 *
 * To download a host software package, please clink on the following link
 * to open the download page in your default browser.
 *
 * http://firmata.org/wiki/Download
 */

/* 
 * This firmware reads all inputs and sends them as fast as it can.  It was
 * inspired by the ease-of-use of the Arduino2Max program.
 *
 * This example code is in the public domain.
 */
#include <Firmata.h>

byte pin;

int analogValue;
int previousAnalogValues[TOTAL_ANALOG_PINS];

byte portStatus[TOTAL_PORTS];	// each bit: 1=pin is digital input, 0=other/ignore
byte previousPINs[TOTAL_PORTS];

/* timer variables */
unsigned long currentMillis;     // store the current value from millis()
unsigned long previousMillis;    // for comparison with currentMillis
/* make sure that the FTDI buffer doesn't go over 60 bytes, otherwise you
   get long, random delays.  So only read analogs every 20ms or so */
int samplingInterval = 19;      // how often to run the main loop (in ms)


/* Rotary */
int encoderPin1 = 2;
int encoderPin2 = 3;
volatile int lastEncoded = 0;
volatile long encoderValue = 0;
long lastencoderValue = 0;
int lastMSB = 0;
int lastLSB = 0;


/*===================
  Functions
====================*/

byte previousPIN[TOTAL_PORTS];  // PIN means PORT for input
byte previousPORT[TOTAL_PORTS]; 

void outputPort(byte portNumber, byte portValue)
{
    // only send the data when it changes, otherwise you get too many messages!
    if (previousPIN[portNumber] != portValue) {
        Firmata.sendDigitalPort(portNumber, portValue); 
        previousPIN[portNumber] = portValue;
    }
}

void setPinModeCallback(byte pin, int mode) {
    if (IS_PIN_DIGITAL(pin)) {
        pinMode(PIN_TO_DIGITAL(pin), mode);
    }
}

void digitalWriteCallback(byte port, int value)
{
    byte i;
    byte currentPinValue, previousPinValue;

    if (port < TOTAL_PORTS && value != previousPORT[port]) {
        for(i=0; i<8; i++) {
            currentPinValue = (byte) value & (1 << i);
            previousPinValue = previousPORT[port] & (1 << i);
            if(currentPinValue != previousPinValue) {
                digitalWrite(i + (port*8), currentPinValue);
            }
        }
        previousPORT[port] = value;
    }
}

void updateEncoder(){
  int MSB = digitalRead(encoderPin1); //MSB = most significant bit
  int LSB = digitalRead(encoderPin2); //LSB = least significant bit

  int encoded = (MSB << 1) |LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value

  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++;
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --;

  lastEncoded = encoded; //store this value for next time
}

void setup()
{
  byte i, port, status;

  Firmata.setFirmwareVersion(0, 1);

  for(pin = 4; pin < TOTAL_PINS; pin++) {
    if IS_PIN_DIGITAL(pin) pinMode(PIN_TO_DIGITAL(pin), INPUT);
  }

  for (port=0; port<TOTAL_PORTS; port++) {
    status = 0;
    for (i=0; i<8; i++) {
      if (IS_PIN_DIGITAL(port * 8 + i)) status |= (1 << i);
    }
    portStatus[port] = status;
  }
  
  /* Rotary */
  pinMode(encoderPin1, INPUT); 
  pinMode(encoderPin2, INPUT);

  digitalWrite(encoderPin1, HIGH); //turn pullup resistor on
  digitalWrite(encoderPin2, HIGH); //turn pullup resistor on

  //call updateEncoder() when any high/low changed seen
  //on interrupt 0 (pin 2), or interrupt 1 (pin 3) 
  attachInterrupt(0, updateEncoder, CHANGE); 
  attachInterrupt(1, updateEncoder, CHANGE);

  Firmata.attach(DIGITAL_MESSAGE, digitalWriteCallback);
  Firmata.attach(SET_PIN_MODE, setPinModeCallback);
  Firmata.begin(57600);
}

void loop()
{
  byte i;

  for (i=0; i<TOTAL_PORTS; i++) {
      outputPort(i, readPort(i, 0xff));
  }
  /* make sure that the FTDI buffer doesn't go over 60 bytes, otherwise you
     get long, random delays.  So only read analogs every 20ms or so */
  currentMillis = millis();
  if(currentMillis - previousMillis > samplingInterval) {
    previousMillis += samplingInterval;
    while(Firmata.available()) {
      Firmata.processInput();
    }
    
    Firmata.sendAnalog(0, encoderValue);
    
    for(pin = 1; pin < TOTAL_ANALOG_PINS; pin++) {
      analogValue = analogRead(pin);
      if(analogValue != previousAnalogValues[pin]) {
        Firmata.sendAnalog(pin, analogValue); 
        previousAnalogValues[pin] = analogValue;
      }
    }
  }
}


