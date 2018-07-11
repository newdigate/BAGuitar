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
#if defined(HAS_KINETIS_SDHC)
		if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStartUsingSPI();
#else
		AudioStartUsingSPI();
#endif
	}

    BAAudioEffectLoopSD::~BAAudioEffectLoopSD()
	{
#if defined(HAS_KINETIS_SDHC)
		if (!(SIM_SCGC3 & SIM_SCGC3_SDHC)) AudioStopUsingSPI();
#else
		AudioStopUsingSPI();
#endif
	}

	void BAAudioEffectLoopSD::delay(float milliseconds) {
		if (m_activeMask)
			return;
		__disable_irq();
		m_activeMask = 1;

		if (milliseconds < 0.0) milliseconds = 0.0;
		uint32_t n = round(milliseconds*(AUDIO_SAMPLE_RATE_EXACT/1000.0f));
		m_channelDelayLength = n;
		_file.close();
		_file = SD.open(_filename, O_READ);
		if (!_file) {
			Serial.print("Cannt open file for read");
			Serial.println(_filename);
			return;
		}
		_filesize = _file.size();
		if (m_channelDelayLength > _filesize / 2)
			m_channelDelayLength = _filesize / 2;
		__enable_irq();
	}

	void BAAudioEffectLoopSD::disable() {
		__disable_irq();
		if (m_activeMask == 0)
			return;

		_file.flush();
		_file.close();
		m_activeMask = 0;

		_file = SD.open(_filename, O_WRITE | O_CREAT | O_TRUNC);
		if (!_file) {
			Serial.print("Cannt open file for write, disable");
			Serial.println(_filename);
			return;
		}
		__enable_irq();
	}

	void BAAudioEffectLoopSD::update(void) {
        if (!_initialized) return;

		audio_block_t *blockIn, *blockOut;

		blockIn = receiveReadOnly();
		blockOut = allocate();
		if (!blockOut) {
			if (blockIn)
				release(blockIn);
			return;
		}

		if (m_activeMask == 0) {
			// pass-through, loop NOT active
			// only record to memory when looping is not active
			if (blockIn) {
				memcpy(blockIn->data, blockOut->data, AUDIO_BLOCK_SAMPLES * 2);
				write(AUDIO_BLOCK_SAMPLES, blockIn->data);
				release(blockIn);
			}
			else {
				memset(blockOut->data, 0, AUDIO_BLOCK_SAMPLES * 2);
				write(AUDIO_BLOCK_SAMPLES, blockOut->data);
			}

		} else {
			release(blockIn);
			read(AUDIO_BLOCK_SAMPLES, blockOut->data);
		}
		transmit(blockOut, 0);
		release(blockOut);
	}

	void BAAudioEffectLoopSD::initialize()
	{
		__disable_irq();
		m_activeMask = 0;

		_file = SD.open(_filename, O_WRITE | O_CREAT | O_TRUNC);
		if (!_file) {
			Serial.print("initialize: Cannt open file for write");
			Serial.println(_filename);
			return;
		}
		Serial.println("_initialized");
		_initialized = true;

		__enable_irq();
	}

	void BAAudioEffectLoopSD::read(uint32_t count, int16_t *data)
	{
		__disable_irq();
		if (_file.available())
			_file.read(data, count*2);
		else
			memset(data, 0, count*2);
		__enable_irq();
	}

	void BAAudioEffectLoopSD::write( uint32_t count, const int16_t *data)
	{
		uint32_t numBytes = count * 2;
		if (_writeBufferIndex <= 16384/4 - numBytes) {
			memcpy(_writebuffer + _writeBufferIndex, data, numBytes);
			_writeBufferIndex += numBytes;
		}
		else {
			__disable_irq();
			_file.write(_writebuffer, _writeBufferIndex);
			__enable_irq();
			_writeBufferIndex = 0;
			memcpy(_writebuffer + _writeBufferIndex, data, numBytes);
			_writeBufferIndex += numBytes;
		}
	}

} /* namespace BAGuitar */
