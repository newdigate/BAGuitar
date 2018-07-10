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

		_file = SD.open(_filename, FILE_READ);
		_filesize = _file.size();
		if (m_channelDelayLength > _filesize / 2)
			m_channelDelayLength = _filesize / 2;
		_reading = true;
		__enable_irq();
	}

	void BAAudioEffectLoopSD::disable() {
		m_activeMask = 0;
		__disable_irq();
		if (_file)
			_file.close();
		_reading = false;

		_file = SD.open(_filename, FILE_WRITE);
		_writing = true;
		__enable_irq();
	}

	void BAAudioEffectLoopSD::update(void) {
        if (!_initialized) return;
        if (!_reading && !_writing) return;

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
				if (_writing) {
					write(AUDIO_BLOCK_SAMPLES, blockIn->data);
				}
				release(blockIn);
			}
			else {
				memset(blockOut->data, 0, AUDIO_BLOCK_SAMPLES);
			}


		} else {
			release(blockIn);
			if (_reading) {
				// compute the delayed location where we read
					read(AUDIO_BLOCK_SAMPLES, blockOut->data);
			}
		}
		transmit(blockOut, 0);
		release(blockOut);
	}

	void BAAudioEffectLoopSD::initialize()
	{
		m_activeMask = 0;

		__disable_irq();
		_file = SD.open(_filename, FILE_WRITE);

		if (!_file) {
			Serial.print("Cannt open file for write");
			Serial.println(_filename);
			return;
		}

		_initialized = true;
		_writing = true;
		Serial.println("_initialized");
		__enable_irq();
	}

	void BAAudioEffectLoopSD::read(uint32_t count, int16_t *data)
	{
		__disable_irq();
		if (!_file.available())
			_file.seek(0);
		_file.read(data, count * 2);
		__enable_irq();
	}

	void BAAudioEffectLoopSD::write( uint32_t count, const int16_t *data)
	{
		__disable_irq();
		_file.write( (byte*)data, count * 2);
		_file.flush();
		__enable_irq();
	}

} /* namespace BAGuitar */
