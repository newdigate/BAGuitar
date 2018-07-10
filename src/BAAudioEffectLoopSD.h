/**************************************************************************//**
 *  @file
 *  @author Steve Lascos
 *  @company Blackaddr Audio
 *
 *  BAAudioEffectDelayExternal is a class for using an external SPI SRAM chip
 *  as an audio delay line. The external memory can be shared among several
 *  different instances of BAAudioEffectDelayExternal by specifying the max delay
 *  length during construction.
 *
 *  @copyright This program is free software: you can redistribute it and/or modify
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
 *****************************************************************************/

#ifndef __BAGUITAR_BAAUDIOEFFECTLOOPSD_H
#define __BAGUITAR_BAAUDIOEFFECTLOOPSD_H

#include <Audio.h>
#include "AudioStream.h"
#include "SD.h"
#include "BAHardware.h"

namespace BAGuitar {

/**************************************************************************//**
 * BAAudioEffectDelayExternal can use external SPI RAM for delay rather than
 * the limited RAM available on the Teensy itself.
 *****************************************************************************/
class BAAudioEffectLoopSD : public AudioStream
{
public:

	/// Default constructor will use all memory available in MEM0
    BAAudioEffectLoopSD();

	/// Specifiy which external memory to use
	/// @param type specify which memory to use
	/// Specify external memory, and how much of the memory to use
	/// @param type specify which memory to use
	/// @param delayLengthMs maximum delay length in milliseconds
	virtual ~BAAudioEffectLoopSD();

	void initialize();

	/// set the actual amount of delay on a given delay tap
	/// @param channel specify channel tap 1-8
	/// @param milliseconds specify how much delay for the specified tap
	void delay(float milliseconds);

	/// Disable a delay channel tap
	/// @param channel specify channel tap 1-8
	void disable();

	virtual void update(void);

private:
    char *_filename = "loop3.raw";
    bool _initialized = false;
    bool _reading = false;
    bool _writing = false;
	void read(uint32_t count, int16_t *data);
	void write(uint32_t count, const int16_t *data);
	File _file;    // the first address in the memory we're using
	unsigned m_channelDelayLength; // # of sample delay for each channel (128 = no delay)
	unsigned  m_activeMask;      // which output channels are active
	audio_block_t *m_inputQueueArray[1];
    uint32_t _filesize;
};


} /* namespace BAGuitar */

#endif /* __BAGUITAR_BAAUDIOEFFECTLOOPSD_H */
