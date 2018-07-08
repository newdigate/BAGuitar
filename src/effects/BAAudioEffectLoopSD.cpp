/*
 * BAAudioEffectDelayExternal.cpp
 *
 *  Created on: November 1, 2017
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

#include "../BAAudioEffectLoopSD.h"

namespace BAGuitar {

    BAAudioEffectLoopSD::BAAudioEffectLoopSD()
	: AudioStream(1, m_inputQueueArray)
	{
	}

    BAAudioEffectLoopSD::~BAAudioEffectLoopSD()
	{
	}

	void BAAudioEffectLoopSD::delay(float milliseconds) {

		if (milliseconds < 0.0) milliseconds = 0.0;
		uint32_t n = round(milliseconds*(AUDIO_SAMPLE_RATE_EXACT/1000.0f));
		//Serial.printf(":samples: %d", n);
		//n += AUDIO_BLOCK_SAMPLES;
		//if (n > m_memoryLength - AUDIO_BLOCK_SAMPLES)
	//		n = m_memoryLength - AUDIO_BLOCK_SAMPLES;
		m_channelDelayLength = n;
		m_activeMask = 1;

		if (m_headOffset >= n )
			read_offset = m_headOffset - n;
		else
			read_offset = m_memoryLength - (n - m_headOffset);
	}

	void BAAudioEffectLoopSD::disable() {
		uint8_t mask = 0;
		m_activeMask = mask;
	}

	void BAAudioEffectLoopSD::update(void) {
		audio_block_t *blockIn, *blockOut;
		uint32_t n;

		blockIn = receiveReadOnly();
		blockOut = allocate();
		if (!blockOut) return;

		if (!m_activeMask) {
			// pass-through, loop NOT active
			// only record to memory when looping is not active
			if (blockIn) {
				memcpy(blockIn->data, blockOut->data, AUDIO_BLOCK_SAMPLES);

				if (m_headOffset + AUDIO_BLOCK_SAMPLES <= m_memoryLength) {
					// a single write is enough
					write(m_headOffset, AUDIO_BLOCK_SAMPLES, blockIn->data);
					m_headOffset += AUDIO_BLOCK_SAMPLES;
				} else {
					// write wraps across end-of-memory
					n = m_memoryLength - m_headOffset;
					write(m_headOffset, n, blockIn->data);
					m_headOffset = AUDIO_BLOCK_SAMPLES - n;
					write(0, m_headOffset, blockIn->data + n);
				}
				release(blockIn);
			}
			else {
				memset(blockOut->data, 0, AUDIO_BLOCK_SAMPLES);

				// if no input, store zeros, so later playback will
				// not be random garbage previously stored in memory
				if (m_headOffset + AUDIO_BLOCK_SAMPLES <= m_memoryLength) {
					zero(m_headOffset, AUDIO_BLOCK_SAMPLES);
					m_headOffset += AUDIO_BLOCK_SAMPLES;
				} else {
					n = m_memoryLength - m_headOffset;
					zero(m_headOffset, n);
					m_headOffset = AUDIO_BLOCK_SAMPLES - n;
					zero(0, m_headOffset);
				}
			}


		} else {
			release(blockIn);
			// compute the delayed location where we read
			if (read_offset + AUDIO_BLOCK_SAMPLES <= m_memoryLength) {
				// a single read will do it
				read(read_offset, AUDIO_BLOCK_SAMPLES, blockOut->data);
				read_offset += AUDIO_BLOCK_SAMPLES;
				if (read_offset >= m_headOffset)
					read_offset = m_headOffset - m_channelDelayLength;
			} else {
				// read wraps across end-of-memory
				n = m_memoryLength - read_offset;
				read(read_offset, n, blockOut->data);
				read(0, AUDIO_BLOCK_SAMPLES - n, blockOut->data + n);
				read_offset = AUDIO_BLOCK_SAMPLES - n;
			}
		}
		transmit(blockOut, 0);
		release(blockOut);
	}

	void BAAudioEffectLoopSD::initialize(float delayLengthMilliseconds) {
		unsigned delayLengthInt = (delayLengthMilliseconds*(AUDIO_SAMPLE_RATE_EXACT/1000.0f))+0.5f;
		initialize(delayLengthInt);
	}
	void BAAudioEffectLoopSD::initialize(unsigned delayLength)
	{
		unsigned samples = 0;
		unsigned memsize = 65535;

		m_activeMask = 0;
		m_headOffset = 0;

		if (delayLength > memsize)
			samples = memsize;

		m_memoryLength = samples;
		//Serial.printf("about to zero: %d\n", m_memoryLength);
		//delay(2000);
		_file = SD.open("/loop.raw", O_RDWR);
		if (!_file) return;
		zero(0, m_memoryLength);
		//Serial.print("zeroed!\n");
		//delay(2000);
	}


	void BAAudioEffectLoopSD::read(uint32_t offset, uint32_t count, int16_t *data)
	{
		_file.seek(offset);
		_file.read(data, count * 2);
	}

	void BAAudioEffectLoopSD::write(uint32_t offset, uint32_t count, const int16_t *data)
	{
		_file.seek(offset);
		_file.write( (byte*)data, count * 2);
	}

	///////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	///////////////////////////////////////////////////////////////////
	void BAAudioEffectLoopSD::zero(uint32_t address, uint32_t count) {
			_file.seek(address);
			int16_t  ints[255];
			memset(ints, 0, 255 * 2);
			unsigned remaining = count;
			while (remaining > 0) {
				unsigned countSamplesToWrite= (remaining >= 255)? 255 : remaining;
				int written = _file.write((byte*)ints, countSamplesToWrite * 2 );
				remaining -= written;
			}
	}




} /* namespace BAGuitar */
