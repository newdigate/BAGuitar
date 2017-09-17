/*
 * BAAudioControlWM8731.h
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
	Wire.begin(); // start the I2C service
}

BAAudioControlWM8731::~BAAudioControlWM8731()
{
}

void BAAudioControlWM8731::disable(void)
{

	// set OUTPD to '1' (powerdown), which is bit 4
	regArray[WM8731_REG_POWERDOWN] |= 0x10;
	write(WM8731_REG_POWERDOWN, regArray[WM8731_REG_POWERDOWN]);
	delay(100); // wait for power down

	// power down the rest of the supplies
	write(WM8731_REG_POWERDOWN, 0x9f); // complete codec powerdown
	delay(100);

	resetCodec();
}


bool BAAudioControlWM8731::enable(void)
{

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

//	// Activate the audio interface
//	setActivate(true);
//	delay(10);

	//setDacMute(false);
	delay(100); // wait for mute ramp

	return true;

}


void BAAudioControlWM8731::setLeftInputGain(int val)
{
	regArray[WM8731_LEFT_INPUT_GAIN_ADDR] &= ~WM8731_LEFT_INPUT_GAIN_MASK;
	regArray[WM8731_LEFT_INPUT_GAIN_ADDR] |=
			((val << WM8731_LEFT_INPUT_GAIN_SHIFT) & WM8731_LEFT_INPUT_GAIN_MASK);
	write(WM8731_LEFT_INPUT_GAIN_ADDR, regArray[WM8731_LEFT_INPUT_GAIN_ADDR]);
}

void BAAudioControlWM8731::setLeftInMute(bool val)
{
	if (val) {
		regArray[WM8731_LEFT_INPUT_MUTE_ADDR] |= WM8731_LEFT_INPUT_MUTE_MASK;
	} else {
		regArray[WM8731_LEFT_INPUT_MUTE_ADDR] &= ~WM8731_LEFT_INPUT_MUTE_MASK;
	}
	write(WM8731_LEFT_INPUT_MUTE_ADDR, regArray[WM8731_LEFT_INPUT_MUTE_ADDR]);
}

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

void BAAudioControlWM8731::setRightInputGain(int val)
{
	regArray[WM8731_RIGHT_INPUT_GAIN_ADDR] &= ~WM8731_RIGHT_INPUT_GAIN_MASK;
	regArray[WM8731_RIGHT_INPUT_GAIN_ADDR] |=
			((val << WM8731_RIGHT_INPUT_GAIN_SHIFT) & WM8731_RIGHT_INPUT_GAIN_MASK);
	write(WM8731_RIGHT_INPUT_GAIN_ADDR, regArray[WM8731_RIGHT_INPUT_GAIN_ADDR]);
}

void BAAudioControlWM8731::setRightInMute(bool val)
{
	if (val) {
		regArray[WM8731_RIGHT_INPUT_MUTE_ADDR] |= WM8731_RIGHT_INPUT_MUTE_MASK;
	} else {
		regArray[WM8731_RIGHT_INPUT_MUTE_ADDR] &= ~WM8731_RIGHT_INPUT_MUTE_MASK;
	}
	write(WM8731_RIGHT_INPUT_MUTE_ADDR, regArray[WM8731_RIGHT_INPUT_MUTE_ADDR]);
}

void BAAudioControlWM8731::setDacMute(bool val)
{
	if (val) {
		regArray[WM8731_DAC_MUTE_ADDR] |= WM8731_DAC_MUTE_MASK;
	} else {
		regArray[WM8731_DAC_MUTE_ADDR] &= ~WM8731_DAC_MUTE_MASK;
	}
	write(WM8731_DAC_MUTE_ADDR, regArray[WM8731_DAC_MUTE_ADDR]);
}

void BAAudioControlWM8731::setDacSelect(bool val)
{
	if (val) {
		regArray[WM8731_DAC_SELECT_ADDR] |= WM8731_DAC_SELECT_MASK;
	} else {
		regArray[WM8731_DAC_SELECT_ADDR] &= ~WM8731_DAC_SELECT_MASK;
	}
	write(WM8731_DAC_SELECT_ADDR, regArray[WM8731_DAC_SELECT_ADDR]);
}

void BAAudioControlWM8731::setAdcBypass(bool val)
{
	if (val) {
		regArray[WM8731_ADC_BYPASS_ADDR] |= WM8731_ADC_BYPASS_MASK;
	} else {
		regArray[WM8731_ADC_BYPASS_ADDR] &= ~WM8731_ADC_BYPASS_MASK;
	}
	write(WM8731_ADC_BYPASS_ADDR, regArray[WM8731_ADC_BYPASS_ADDR]);
}

void BAAudioControlWM8731::setHPFDisable(bool val)
{
	if (val) {
		regArray[WM8731_HPF_DISABLE_ADDR] |= WM8731_HPF_DISABLE_MASK;
	} else {
		regArray[WM8731_HPF_DISABLE_ADDR] &= ~WM8731_HPF_DISABLE_MASK;
	}
	write(WM8731_HPF_DISABLE_ADDR, regArray[WM8731_HPF_DISABLE_ADDR]);
}

void BAAudioControlWM8731::setActivate(bool val)
{
	if (val) {
		write(WM8731_ACTIVATE_ADDR, WM8731_ACTIVATE_MASK);
	} else {
		write(WM8731_ACTIVATE_ADDR, 0);
	}

}


void BAAudioControlWM8731::resetCodec(void)
{
	write(WM8731_REG_RESET, 0x0);
	resetInternalReg();
}

void BAAudioControlWM8731::writeI2C(unsigned int addr, unsigned int val)
{
	write(addr, val);
}

bool BAAudioControlWM8731::write(unsigned int reg, unsigned int val)
{
	Wire.beginTransmission(WM8731_I2C_ADDR);
	Wire.write((reg << 1) | ((val >> 8) & 1));
	Wire.write(val & 0xFF);
	Wire.endTransmission();
//        Serial.print(String("Wrote "));
//        Serial.print((reg << 1) | ((val >> 8) & 1), HEX);
//        Serial.print(" ");
//        Serial.print(val & 0xFF, HEX);
//        Serial.print("\n");

	return true;
}

} /* namespace BAGuitar */
