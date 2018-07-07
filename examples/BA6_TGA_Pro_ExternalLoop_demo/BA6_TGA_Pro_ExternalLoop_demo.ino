/*************************************************************************
 * This demo uses the BAGuitar library to provide enhanced control of
 * the TGA Pro board.
 * 
 * The latest copy of the BA Guitar library can be obtained from
 * https://github.com/Blackaddr/BAGuitar
 * 
 * This demo shows to use the BAAudioEffectDelayExternal audio class
 * to create long delays using external SPI RAM.
 * 
 * Eight delay taps are configured from a single SRAM device.
 * 
 */

#include "BAGuitar.h"

using namespace BAGuitar;

AudioInputI2S            i2sIn;
AudioOutputI2S           i2sOut;

BAAudioControlWM8731        codecControl;
BAAudioEffectLoopExternal  longDelay(MemSelect::MEM0); // comment this line to use MEM1

AudioConnection  fromInput(i2sIn,0, longDelay, 0);

AudioConnection  fromDelayA(longDelay, 0, i2sOut, 0);
AudioConnection  fromDelayB(longDelay, 0, i2sOut, 1);

void setup() {

  Serial.begin(57600);
  AudioMemory(256);


  Serial.println("Enabling codec...\n");
  codecControl.enable();
  delay(100);

  // Set up 8 delay taps, each with evenly spaced, longer delays
  //longDelay.delay(1000.0f);


}
unsigned lastMillis = 0;
bool delayIsOn = false;
void loop() {

  unsigned millis2 = millis();
  if (millis2 - lastMillis  > 5000 && !delayIsOn) {
    longDelay.delay(250.0f);
    delayIsOn = true;
    Serial.println("Turned on");
  }
  else if  (millis2 - lastMillis > 10000 && delayIsOn) {
    longDelay.disable();
    delayIsOn = false;
    lastMillis = millis2;
    Serial.println("Turned off");
  }
}
