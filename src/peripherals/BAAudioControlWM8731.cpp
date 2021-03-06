/*
 * BAAudioControlWM8731.cpp
 *
 *  Created on: May 22, 2017
 *      Author: slascos
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.*
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <Wire.h>
#include "BAAudioControlWM8731.h"

namespace BAGuitar {

// use const instead of define for proper scoping
constexpr int WM8731_I2C_ADDR = 0x1A;

// The WM8731 register map
constexpr int WM8731_REG_LLINEIN   = 0;
constexpr int WM8731_REG_RLINEIN   = 1;
constexpr int WM8731_REG_LHEADOUT  = 2;
constexpr int WM8731_REG_RHEADOUT  = 3;
constexpr int WM8731_REG_ANALOG     =4;
constexpr int WM8731_REG_DIGITAL   = 5;
constexpr int WM8731_REG_POWERDOWN = 6;
constexpr int WM8731_REG_INTERFACE = 7;
constexpr int WM8731_REG_SAMPLING  = 8;
constexpr int WM8731_REG_ACTIVE    = 9;
constexpr int WM8731_REG_RESET    = 15;

// Register Masks and Shifts
// Register 0
constexpr int WM8731_LEFT_INPUT_GAIN_ADDR = 0;
constexpr int WM8731_LEFT_INPUT_GAIN_MASK = 0x1F;
constexpr int WM8731_LEFT_INPUT_GAIN_SHIFT = 0;
constexpr int WM8731_LEFT_INPUT_MUTE_ADDR = 0;
constexpr int WM8731_LEFT_INPUT_MUTE_MASK = 0x80;
constexpr int WM8731_LEFT_INPUT_MUTE_SHIFT = 7;
constexpr int WM8731_LINK_LEFT_RIGHT_IN_ADDR = 0;
constexpr int WM8731_LINK_LEFT_RIGHT_IN_MASK = 0x100;
constexpr int WM8731_LINK_LEFT_RIGHT_IN_SHIFT = 8;
// Register 1
constexpr int WM8731_RIGHT_INPUT_GAIN_ADDR = 1;
constexpr int WM8731_RIGHT_INPUT_GAIN_MASK = 0x1F;
constexpr int WM8731_RIGHT_INPUT_GAIN_SHIFT = 0;
constexpr int WM8731_RIGHT_INPUT_MUTE_ADDR = 1;
constexpr int WM8731_RIGHT_INPUT_MUTE_MASK = 0x80;
constexpr int WM8731_RIGHT_INPUT_MUTE_SHIFT = 7;
constexpr int WM8731_LINK_RIGHT_LEFT_IN_ADDR = 1;
constexpr int WM8731_LINK_RIGHT_LEFT_IN_MASK = 0x100;
constexpr int WM8731_LINK_RIGHT_LEFT_IN_SHIFT = 8;
// Register 4
constexpr int WM8731_ADC_BYPASS_ADDR = 4;
constexpr int WM8731_ADC_BYPASS_MASK = 0x8;
constexpr int WM8731_ADC_BYPASS_SHIFT = 3;
constexpr int WM8731_DAC_SELECT_ADDR = 4;
constexpr int WM8731_DAC_SELECT_MASK = 0x10;
constexpr int WM8731_DAC_SELECT_SHIFT = 4;
// Register 5
constexpr int WM8731_DAC_MUTE_ADDR = 5;
constexpr int WM8731_DAC_MUTE_MASK = 0x8;
constexpr int WM8731_DAC_MUTE_SHIFT = 3;
constexpr int WM8731_HPF_DISABLE_ADDR = 5;
constexpr int WM8731_HPF_DISABLE_MASK = 0x1;
constexpr int WM8731_HPF_DISABLE_SHIFT = 0;
// Register 7
constexpr int WM8731_LRSWAP_ADDR = 5;
constexpr int WM8731_LRSWAP_MASK = 0x20;
constexpr int WM8731_LRSWAPE_SHIFT = 5;

// Register 9
constexpr int WM8731_ACTIVATE_ADDR = 9;
constexpr int WM8731_ACTIVATE_MASK = 0x1;


// Reset the internal shadow register array to match
// the reset state of the codec.
void BAAudioControlWM8731::resetInternalReg(void) {
	// Set to reset state
	regArray[0] = 0x97;
	regArray[1] = 0x97;
	regArray[2] = 0x79;
	regArray[3] = 0x79;
	regArray[4] = 0x0a;
	regArray[5] = 0x8;
	regArray[6] = 0x9f;
	regArray[7] = 0xa;
	regArray[8] = 0;
	regArray[9] = 0;
}

BAAudioControlWM8731::BAAudioControlWM8731()
{
	resetInternalReg();
}

BAAudioControlWM8731::~BAAudioControlWM8731()
{
}

// Powerdown and disable the codec
void BAAudioControlWM8731::disable(void)
{

	//Serial.println("Disabling codec");
	if (m_wireStarted == false) { Wire.begin(); m_wireStarted = true; }

	// set OUTPD to '1' (powerdown), which is bit 4
	regArray[WM8731_REG_POWERDOWN] |= 0x10;
	write(WM8731_REG_POWERDOWN, regArray[WM8731_REG_POWERDOWN]);
	delay(100); // wait for power down

	// power down the rest of the supplies
	write(WM8731_REG_POWERDOWN, 0x9f); // complete codec powerdown
	delay(100);

	resetCodec();
}

// Powerup and unmute the codec
void BAAudioControlWM8731::enable(void)
{

	disable(); // disable first in case it was already powered up

	//Serial.println("Enabling codec");
	if (m_wireStarted == false) { Wire.begin(); m_wireStarted = true; }
	// Sequence from WAN0111.pdf

	// Begin configuring the codec
	resetCodec();
	delay(100); // wait for reset

	// Power up all domains except OUTPD and microphone
	regArray[WM8731_REG_POWERDOWN] = 0x12;
	write(WM8731_REG_POWERDOWN, regArray[WM8731_REG_POWERDOWN]);
	delay(100); // wait for codec powerup


	setAdcBypass(false); // causes a slight click
	setDacSelect(true);
	setHPFDisable(true);
	setLeftInputGain(0x17); // default input gain
    setRightInputGain(0x17);
	setLeftInMute(false); // no input mute
	setRightInMute(false);
	setDacMute(false); // unmute the DAC

	// mute the headphone outputs
	write(WM8731_REG_LHEADOUT, 0x00);      // volume off
	regArray[WM8731_REG_LHEADOUT] = 0x00;
	write(WM8731_REG_RHEADOUT, 0x00);
	regArray[WM8731_REG_RHEADOUT] = 0x00;

	/// Configure the audio interface
	write(WM8731_REG_INTERFACE, 0x02); // I2S, 16 bit, MCLK slave
	regArray[WM8731_REG_INTERFACE] = 0x2;

	write(WM8731_REG_SAMPLING, 0x20);  // 256*Fs, 44.1 kHz, MCLK/1
	regArray[WM8731_REG_SAMPLING] = 0x20;
	delay(100); // wait for interface config

	// Activate the audio interface
	setActivate(true);
	delay(100);

	write(WM8731_REG_POWERDOWN, 0x02); // power up outputs
	regArray[WM8731_REG_POWERDOWN] = 0x02;
	delay(500); // wait for output to power up

	//Serial.println("Done codec config");


	delay(100); // wait for mute ramp

}

// Set the PGA gain on the Left channel
void BAAudioControlWM8731::setLeftInputGain(int val)
{
	regArray[WM8731_LEFT_INPUT_GAIN_ADDR] &= ~WM8731_LEFT_INPUT_GAIN_MASK;
	regArray[WM8731_LEFT_INPUT_GAIN_ADDR] |=
			((val << WM8731_LEFT_INPUT_GAIN_SHIFT) & WM8731_LEFT_INPUT_GAIN_MASK);
	write(WM8731_LEFT_INPUT_GAIN_ADDR, regArray[WM8731_LEFT_INPUT_GAIN_ADDR]);
}

// Mute control on the ADC Left channel
void BAAudioControlWM8731::setLeftInMute(bool val)
{
	if (val) {
		regArray[WM8731_LEFT_INPUT_MUTE_ADDR] |= WM8731_LEFT_INPUT_MUTE_MASK;
	} else {
		regArray[WM8731_LEFT_INPUT_MUTE_ADDR] &= ~WM8731_LEFT_INPUT_MUTE_MASK;
	}
	write(WM8731_LEFT_INPUT_MUTE_ADDR, regArray[WM8731_LEFT_INPUT_MUTE_ADDR]);
}

// Link the gain/mute controls for Left and Right channels
void BAAudioControlWM8731::setLinkLeftRightIn(bool val)
{
	if (val) {
		regArray[WM8731_LINK_LEFT_RIGHT_IN_ADDR] |= WM8731_LINK_LEFT_RIGHT_IN_MASK;
		regArray[WM8731_LINK_RIGHT_LEFT_IN_ADDR] |= WM8731_LINK_RIGHT_LEFT_IN_MASK;
	} else {
		regArray[WM8731_LINK_LEFT_RIGHT_IN_ADDR] &= ~WM8731_LINK_LEFT_RIGHT_IN_MASK;
		regArray[WM8731_LINK_RIGHT_LEFT_IN_ADDR] &= ~WM8731_LINK_RIGHT_LEFT_IN_MASK;
	}
	write(WM8731_LINK_LEFT_RIGHT_IN_ADDR, regArray[WM8731_LINK_LEFT_RIGHT_IN_ADDR]);
	write(WM8731_LINK_RIGHT_LEFT_IN_ADDR, regArray[WM8731_LINK_RIGHT_LEFT_IN_ADDR]);
}

// Set the PGA input gain on the Right channel
void BAAudioControlWM8731::setRightInputGain(int val)
{
	regArray[WM8731_RIGHT_INPUT_GAIN_ADDR] &= ~WM8731_RIGHT_INPUT_GAIN_MASK;
	regArray[WM8731_RIGHT_INPUT_GAIN_ADDR] |=
			((val << WM8731_RIGHT_INPUT_GAIN_SHIFT) & WM8731_RIGHT_INPUT_GAIN_MASK);
	write(WM8731_RIGHT_INPUT_GAIN_ADDR, regArray[WM8731_RIGHT_INPUT_GAIN_ADDR]);
}

// Mute control on the input ADC right channel
void BAAudioControlWM8731::setRightInMute(bool val)
{
	if (val) {
		regArray[WM8731_RIGHT_INPUT_MUTE_ADDR] |= WM8731_RIGHT_INPUT_MUTE_MASK;
	} else {
		regArray[WM8731_RIGHT_INPUT_MUTE_ADDR] &= ~WM8731_RIGHT_INPUT_MUTE_MASK;
	}
	write(WM8731_RIGHT_INPUT_MUTE_ADDR, regArray[WM8731_RIGHT_INPUT_MUTE_ADDR]);
}

// Left/right swap control
void BAAudioControlWM8731::setLeftRightSwap(bool val)
{
	if (val) {
		regArray[WM8731_LRSWAP_ADDR] |= WM8731_LRSWAP_MASK;
	} else {
		regArray[WM8731_LRSWAP_ADDR] &= ~WM8731_LRSWAP_MASK;
	}
	write(WM8731_LRSWAP_ADDR, regArray[WM8731_LRSWAP_ADDR]);
}

// Dac output mute control
void BAAudioControlWM8731::setDacMute(bool val)
{
	if (val) {
		regArray[WM8731_DAC_MUTE_ADDR] |= WM8731_DAC_MUTE_MASK;
	} else {
		regArray[WM8731_DAC_MUTE_ADDR] &= ~WM8731_DAC_MUTE_MASK;
	}
	write(WM8731_DAC_MUTE_ADDR, regArray[WM8731_DAC_MUTE_ADDR]);
}

// Switches the DAC audio in/out of the output path
void BAAudioControlWM8731::setDacSelect(bool val)
{
	if (val) {
		regArray[WM8731_DAC_SELECT_ADDR] |= WM8731_DAC_SELECT_MASK;
	} else {
		regArray[WM8731_DAC_SELECT_ADDR] &= ~WM8731_DAC_SELECT_MASK;
	}
	write(WM8731_DAC_SELECT_ADDR, regArray[WM8731_DAC_SELECT_ADDR]);
}

// Bypass sends the ADC input audio (analog) directly to analog output stage
// bypassing all digital processing
void BAAudioControlWM8731::setAdcBypass(bool val)
{
	if (val) {
		regArray[WM8731_ADC_BYPASS_ADDR] |= WM8731_ADC_BYPASS_MASK;
	} else {
		regArray[WM8731_ADC_BYPASS_ADDR] &= ~WM8731_ADC_BYPASS_MASK;
	}
	write(WM8731_ADC_BYPASS_ADDR, regArray[WM8731_ADC_BYPASS_ADDR]);
}

// Enable/disable the dynamic HPF (recommended, it creates noise)
void BAAudioControlWM8731::setHPFDisable(bool val)
{
	if (val) {
		regArray[WM8731_HPF_DISABLE_ADDR] |= WM8731_HPF_DISABLE_MASK;
	} else {
		regArray[WM8731_HPF_DISABLE_ADDR] &= ~WM8731_HPF_DISABLE_MASK;
	}
	write(WM8731_HPF_DISABLE_ADDR, regArray[WM8731_HPF_DISABLE_ADDR]);
}

// Activate/deactive the I2S audio interface
void BAAudioControlWM8731::setActivate(bool val)
{
	if (val) {
		write(WM8731_ACTIVATE_ADDR, WM8731_ACTIVATE_MASK);
	} else {
		write(WM8731_ACTIVATE_ADDR, 0);
	}

}

// Trigger the on-chip codec reset
void BAAudioControlWM8731::resetCodec(void)
{
	write(WM8731_REG_RESET, 0x0);
	resetInternalReg();
}

// Direct write control to the codec
bool BAAudioControlWM8731::writeI2C(unsigned int addr, unsigned int val)
{
	return write(addr, val);
}

// Low level write control for the codec via the Teensy I2C interface
bool BAAudioControlWM8731::write(unsigned int reg, unsigned int val)
{
	bool done = false;

	while (!done) {
		Wire.beginTransmission(WM8731_I2C_ADDR);
		Wire.write((reg << 1) | ((val >> 8) & 1));
		Wire.write(val & 0xFF);
		if (byte error = Wire.endTransmission() ) {
			(void)error; // supress warning about unused variable
			//Serial.println(String("Wire::Error: ") + error + String(" retrying..."));
		} else {
			done = true;
			//Serial.println("Wire::SUCCESS!");
		}
	}

	return true;
}

} /* namespace BAGuitar */
