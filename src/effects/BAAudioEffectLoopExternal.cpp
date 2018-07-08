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

#include "../BAAudioEffectLoopExternal.h"

namespace BAGuitar {

#define SPISETTING SPISettings(20000000, MSBFIRST, SPI_MODE0)

struct MemSpiConfig {
	unsigned mosiPin;
	unsigned misoPin;
	unsigned sckPin;
	unsigned csPin;
	unsigned memSize;
};

constexpr MemSpiConfig Mem0Config = {7,  8, 14, 15, 65536 };
constexpr MemSpiConfig Mem1Config = {21, 5, 20, 31, 65536 };

unsigned BAAudioEffectLoopExternal::m_usingSPICount[2] = {0,0};

    BAAudioEffectLoopExternal::BAAudioEffectLoopExternal()
: AudioStream(1, m_inputQueueArray)
{
	initialize(MemSelect::MEM0);
}

    BAAudioEffectLoopExternal::BAAudioEffectLoopExternal(MemSelect mem)
: AudioStream(1, m_inputQueueArray)
{
	initialize(mem);
}

BAAudioEffectLoopExternal::BAAudioEffectLoopExternal(BAGuitar::MemSelect type, float delayLengthMs)
: AudioStream(1, m_inputQueueArray)
{
	unsigned delayLengthInt = (delayLengthMs*(AUDIO_SAMPLE_RATE_EXACT/1000.0f))+0.5f;
	initialize(type, delayLengthInt);
}

    BAAudioEffectLoopExternal::~BAAudioEffectLoopExternal()
{
	if (m_spi) delete m_spi;
}

void BAAudioEffectLoopExternal::delay(float milliseconds) {

	if (milliseconds < 0.0) milliseconds = 0.0;
	uint32_t n = round(milliseconds*(AUDIO_SAMPLE_RATE_EXACT/1000.0f));
	Serial.printf(":samples: %d", n);
	//n += AUDIO_BLOCK_SAMPLES;
	//if (n > m_memoryLength - AUDIO_BLOCK_SAMPLES)
//		n = m_memoryLength - AUDIO_BLOCK_SAMPLES;
	m_channelDelayLength = n;
	if (m_activeMask == 0) m_startUsingSPI(m_spiChannel);
	m_activeMask = 1;

	if (m_headOffset >= n )
		read_offset = m_headOffset - n;
	else
		read_offset = m_memoryLength - (n - m_headOffset);
}

void BAAudioEffectLoopExternal::disable() {
	uint8_t mask = 0;
	m_activeMask = mask;
	if (mask == 0) m_stopUsingSPI(m_spiChannel);
}

void BAAudioEffectLoopExternal::update(void)
{
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

void BAAudioEffectLoopExternal::initialize(MemSelect mem, unsigned delayLength)
{
	unsigned samples = 0;
	unsigned memsize, avail;

	m_activeMask = 0;
	m_headOffset = 0;
	m_mem = mem;

	switch (mem) {
	case MemSelect::MEM0 :
	{
		memsize = Mem0Config.memSize;
		m_spi = &SPI;
		m_spiChannel = 0;
		m_misoPin = Mem0Config.misoPin;
		m_mosiPin = Mem0Config.mosiPin;
		m_sckPin =  Mem0Config.sckPin;
		m_csPin = Mem0Config.csPin;

		m_spi->setMOSI(m_mosiPin);
		m_spi->setMISO(m_misoPin);
		m_spi->setSCK(m_sckPin);
		m_spi->begin();
		break;
	}
	case MemSelect::MEM1 :
	{
#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
		memsize = Mem1Config.memSize;
		m_spi = &SPI1;
		m_spiChannel = 1;
		m_misoPin = Mem1Config.misoPin;
		m_mosiPin = Mem1Config.mosiPin;
		m_sckPin =  Mem1Config.sckPin;
		m_csPin = Mem1Config.csPin;

		m_spi->setMOSI(m_mosiPin);
		m_spi->setMISO(m_misoPin);
		m_spi->setSCK(m_sckPin);
		m_spi->begin();
#endif
		break;
	}

	}

	pinMode(m_csPin, OUTPUT);
	digitalWriteFast(m_csPin, HIGH);

	avail = memsize;
	//Serial.printf("avail: %d\n", avail);

	if (delayLength > avail) samples = avail;
	m_memoryStart = 0;
	m_memoryLength = samples;
    //Serial.printf("about to zero: %d\n", m_memoryLength);
    delay(2000);
	zero(0, m_memoryLength);
    //Serial.print("zeroed!\n");
    delay(2000);
}


void BAAudioEffectLoopExternal::read(uint32_t offset, uint32_t count, int16_t *data)
{
	uint32_t addr = m_memoryStart + offset;
	addr *= 2;

	m_spi->beginTransaction(SPISETTING);
	digitalWriteFast(m_csPin, LOW);
	m_spi->transfer16((0x03 << 8) | (addr >> 16));
	m_spi->transfer16(addr & 0xFFFF);

	while (count) {
		*data++ = (int16_t)(m_spi->transfer16(0));
		count--;
	}
	digitalWriteFast(m_csPin, HIGH);
	m_spi->endTransaction();

}

void BAAudioEffectLoopExternal::write(uint32_t offset, uint32_t count, const int16_t *data)
{
	uint32_t addr = m_memoryStart + offset;

	addr *= 2;
	m_spi->beginTransaction(SPISETTING);
	digitalWriteFast(m_csPin, LOW);
	m_spi->transfer16((0x02 << 8) | (addr >> 16));
	m_spi->transfer16(addr & 0xFFFF);
	while (count) {
		int16_t w = 0;
		if (data) w = *data++;
		m_spi->transfer16(w);
		count--;
	}
	digitalWriteFast(m_csPin, HIGH);
	m_spi->endTransaction();

}

///////////////////////////////////////////////////////////////////
// PRIVATE METHODS
///////////////////////////////////////////////////////////////////
void BAAudioEffectLoopExternal::zero(uint32_t address, uint32_t count) {
	write(address, count, NULL);
}

#ifdef SPI_HAS_NOTUSINGINTERRUPT
inline void BAAudioEffectLoopExternal::m_startUsingSPI(int spiBus) {
	if (spiBus == 0) {
		m_spi->usingInterrupt(IRQ_SOFTWARE);
	} else if (spiBus == 1) {
		m_spi->usingInterrupt(IRQ_SOFTWARE);
	}
	m_usingSPICount[spiBus]++;
}

inline void BAAudioEffectLoopExternal::m_stopUsingSPI(int spiBus) {
	if (m_usingSPICount[spiBus] == 0 || --m_usingSPICount[spiBus] == 0)
	{
		if (spiBus == 0) {
			m_spi->notUsingInterrupt(IRQ_SOFTWARE);
		} else if (spiBus == 1) {
			m_spi->notUsingInterrupt(IRQ_SOFTWARE);
		}

	}
}

#else
inline void BAAudioEffectLoopExternal::m_startUsingSPI(int spiBus) {
	if (spiBus == 0) {
		m_spi->usingInterrupt(IRQ_SOFTWARE);
	} else if (spiBus == 1) {
		m_spi->usingInterrupt(IRQ_SOFTWARE);
	}

}
inline void BAAudioEffectLoopExternal::m_stopUsingSPI(int spiBus) {
}

#endif


} /* namespace BAGuitar */
