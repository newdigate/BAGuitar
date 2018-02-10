/*
 * BASpiMemory.cpp
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

#include "Arduino.h"
#include "BASpiMemory.h"

namespace BAGuitar {

// MEM0 Settings
constexpr int SPI_CS_MEM0 = 15;
constexpr int SPI_MOSI_MEM0 = 7;
constexpr int SPI_MISO_MEM0 = 8;
constexpr int SPI_SCK_MEM0 = 14;

// MEM1 Settings
constexpr int SPI_CS_MEM1 = 31;
constexpr int SPI_MOSI_MEM1 = 21;
constexpr int SPI_MISO_MEM1 = 5;
constexpr int SPI_SCK_MEM1 = 20;

// SPI Constants
constexpr int SPI_WRITE_MODE_REG = 0x1;
constexpr int SPI_WRITE_CMD = 0x2;
constexpr int SPI_READ_CMD = 0x3;
constexpr int SPI_ADDR_2_MASK = 0xFF0000;
constexpr int SPI_ADDR_2_SHIFT = 16;
constexpr int SPI_ADDR_1_MASK = 0x00FF00;
constexpr int SPI_ADDR_1_SHIFT = 8;
constexpr int SPI_ADDR_0_MASK = 0x0000FF;


BASpiMemory::BASpiMemory(SpiDeviceId memDeviceId)
{
	m_memDeviceId = memDeviceId;
	m_settings = {20000000, MSBFIRST, SPI_MODE0};
}

BASpiMemory::BASpiMemory(SpiDeviceId memDeviceId, uint32_t speedHz)
{
	m_memDeviceId = memDeviceId;
	m_settings = {speedHz, MSBFIRST, SPI_MODE0};
}

// Intitialize the correct Arduino SPI interface
void BASpiMemory::begin()
{
	switch (m_memDeviceId) {
	case SpiDeviceId::SPI_DEVICE0 :
		m_csPin = SPI_CS_MEM0;
		m_spi = &SPI;
		m_spi->setMOSI(SPI_MOSI_MEM0);
		m_spi->setMISO(SPI_MISO_MEM0);
		m_spi->setSCK(SPI_SCK_MEM0);
		m_spi->begin();
		break;

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
	case SpiDeviceId::SPI_DEVICE1 :
		m_csPin = SPI_CS_MEM1;
		m_spi = &SPI1;
		m_spi->setMOSI(SPI_MOSI_MEM1);
		m_spi->setMISO(SPI_MISO_MEM1);
		m_spi->setSCK(SPI_SCK_MEM1);
		m_spi->begin();
		break;
#endif

	default :
		// unreachable since memDeviceId is an enumerated class
		return;
	}

	pinMode(m_csPin, OUTPUT);
	digitalWrite(m_csPin, HIGH);
	m_started = true;

}

BASpiMemory::~BASpiMemory() {
}

// Single address write
void BASpiMemory::write(size_t address, uint8_t data)
{
	m_spi->beginTransaction(m_settings);
	digitalWrite(m_csPin, LOW);
	m_spi->transfer(SPI_WRITE_CMD);
	m_spi->transfer((address & SPI_ADDR_2_MASK) >> SPI_ADDR_2_SHIFT);
	m_spi->transfer((address & SPI_ADDR_1_MASK) >> SPI_ADDR_1_SHIFT);
	m_spi->transfer((address & SPI_ADDR_0_MASK));
	m_spi->transfer(data);
	m_spi->endTransaction();
	digitalWrite(m_csPin, HIGH);
}

// Single address write
void BASpiMemory::write(size_t address, uint8_t *data, size_t numBytes)
{
	uint8_t *dataPtr = data;

	m_spi->beginTransaction(m_settings);
	digitalWrite(m_csPin, LOW);
	m_spi->transfer(SPI_WRITE_CMD);
	m_spi->transfer((address & SPI_ADDR_2_MASK) >> SPI_ADDR_2_SHIFT);
	m_spi->transfer((address & SPI_ADDR_1_MASK) >> SPI_ADDR_1_SHIFT);
	m_spi->transfer((address & SPI_ADDR_0_MASK));

	for (size_t i=0; i < numBytes; i++) {
		m_spi->transfer(*dataPtr++);
	}
	m_spi->endTransaction();
	digitalWrite(m_csPin, HIGH);
}


void BASpiMemory::zero(size_t address, size_t numBytes)
{
	m_spi->beginTransaction(m_settings);
	digitalWrite(m_csPin, LOW);
	m_spi->transfer(SPI_WRITE_CMD);
	m_spi->transfer((address & SPI_ADDR_2_MASK) >> SPI_ADDR_2_SHIFT);
	m_spi->transfer((address & SPI_ADDR_1_MASK) >> SPI_ADDR_1_SHIFT);
	m_spi->transfer((address & SPI_ADDR_0_MASK));

	for (size_t i=0; i < numBytes; i++) {
		m_spi->transfer(0);
	}
	m_spi->endTransaction();
	digitalWrite(m_csPin, HIGH);
}

void BASpiMemory::write16(size_t address, uint16_t data)
{
	m_spi->beginTransaction(m_settings);
	digitalWrite(m_csPin, LOW);
	m_spi->transfer16((SPI_WRITE_CMD << 8) | (address >> 16) );
	m_spi->transfer16(address & 0xFFFF);
	m_spi->transfer16(data);
	m_spi->endTransaction();
	digitalWrite(m_csPin, HIGH);
}

void BASpiMemory::write16(size_t address, uint16_t *data, size_t numWords)
{
	uint16_t *dataPtr = data;

	m_spi->beginTransaction(m_settings);
	digitalWrite(m_csPin, LOW);
	m_spi->transfer16((SPI_WRITE_CMD << 8) | (address >> 16) );
	m_spi->transfer16(address & 0xFFFF);

	for (size_t i=0; i<numWords; i++) {
		m_spi->transfer16(*dataPtr++);
	}

	m_spi->endTransaction();
	digitalWrite(m_csPin, HIGH);
}

void BASpiMemory::zero16(size_t address, size_t numWords)
{
	m_spi->beginTransaction(m_settings);
	digitalWrite(m_csPin, LOW);
	m_spi->transfer16((SPI_WRITE_CMD << 8) | (address >> 16) );
	m_spi->transfer16(address & 0xFFFF);

	for (size_t i=0; i<numWords; i++) {
		m_spi->transfer16(0);
	}

	m_spi->endTransaction();
	digitalWrite(m_csPin, HIGH);
	Serial.println("DONE!");
}

// single address read
uint8_t BASpiMemory::read(size_t address)
{
	int data;

	m_spi->beginTransaction(m_settings);
	digitalWrite(m_csPin, LOW);
	m_spi->transfer(SPI_READ_CMD);
	m_spi->transfer((address & SPI_ADDR_2_MASK) >> SPI_ADDR_2_SHIFT);
	m_spi->transfer((address & SPI_ADDR_1_MASK) >> SPI_ADDR_1_SHIFT);
	m_spi->transfer((address & SPI_ADDR_0_MASK));
	data = m_spi->transfer(0);
	m_spi->endTransaction();
	digitalWrite(m_csPin, HIGH);
	return data;
}


void BASpiMemory::read(size_t address, uint8_t *data, size_t numBytes)
{
	uint8_t *dataPtr = data;

	m_spi->beginTransaction(m_settings);
	digitalWrite(m_csPin, LOW);
	m_spi->transfer(SPI_READ_CMD);
	m_spi->transfer((address & SPI_ADDR_2_MASK) >> SPI_ADDR_2_SHIFT);
	m_spi->transfer((address & SPI_ADDR_1_MASK) >> SPI_ADDR_1_SHIFT);
	m_spi->transfer((address & SPI_ADDR_0_MASK));

	for (size_t i=0; i<numBytes; i++) {
		*dataPtr++ = m_spi->transfer(0);
	}

	m_spi->endTransaction();
	digitalWrite(m_csPin, HIGH);
}

uint16_t BASpiMemory::read16(size_t address)
{

	uint16_t data;
	m_spi->beginTransaction(m_settings);
	digitalWrite(m_csPin, LOW);
	m_spi->transfer16((SPI_READ_CMD << 8) | (address >> 16) );
	m_spi->transfer16(address & 0xFFFF);
	data = m_spi->transfer16(0);
	m_spi->endTransaction();

	digitalWrite(m_csPin, HIGH);
	return data;
}

void BASpiMemory::read16(size_t address, uint16_t *dest, size_t numWords)
{

	uint16_t *dataPtr = dest;
	m_spi->beginTransaction(m_settings);
	digitalWrite(m_csPin, LOW);
	m_spi->transfer16((SPI_READ_CMD << 8) | (address >> 16) );
	m_spi->transfer16(address & 0xFFFF);

	for (size_t i=0; i<numWords; i++) {
		*dataPtr++ = m_spi->transfer16(0);
	}

	m_spi->endTransaction();
	digitalWrite(m_csPin, HIGH);
}

/////////////////////////////////////////////////////////////////////////////
// BASpiMemoryDMA
/////////////////////////////////////////////////////////////////////////////
BASpiMemoryDMA::BASpiMemoryDMA(SpiDeviceId memDeviceId, size_t bufferSizeBytes)
: BASpiMemory(memDeviceId), m_bufferSize(bufferSizeBytes)
{
	int cs;
	switch (memDeviceId) {
	case SpiDeviceId::SPI_DEVICE0 :
		cs = SPI_CS_MEM0;
		break;
	case SpiDeviceId::SPI_DEVICE1 :
		cs = SPI_CS_MEM1;
		break;
	default :
		cs = SPI_CS_MEM0;
	}
	m_cs = new ActiveLowChipSelect(cs, m_settings);
	// add 4 bytes to buffer for SPI CMD and 3 bytes of addresse
	m_txBuffer = new uint8_t[bufferSizeBytes+4];
	m_rxBuffer = new uint8_t[bufferSizeBytes+4];
}

BASpiMemoryDMA::BASpiMemoryDMA(SpiDeviceId memDeviceId, uint32_t speedHz, size_t bufferSizeBytes)
: BASpiMemory(memDeviceId, speedHz), m_bufferSize(bufferSizeBytes)
{
	int cs;
	switch (memDeviceId) {
	case SpiDeviceId::SPI_DEVICE0 :
		cs = SPI_CS_MEM0;
		break;
	case SpiDeviceId::SPI_DEVICE1 :
		cs = SPI_CS_MEM1;
		break;
	default :
		cs = SPI_CS_MEM0;
	}
	m_cs = new ActiveLowChipSelect(cs, m_settings);
	m_txBuffer = new uint8_t[bufferSizeBytes+4];
	m_rxBuffer = new uint8_t[bufferSizeBytes+4];
}

BASpiMemoryDMA::~BASpiMemoryDMA()
{
	delete m_cs;
	if (m_txBuffer) delete [] m_txBuffer;
	if (m_rxBuffer) delete [] m_rxBuffer;
}

void BASpiMemoryDMA::m_setSpiCmdAddr(int command, size_t address, uint8_t *dest)
{
	dest[0] = command;
	dest[1] = ((address & SPI_ADDR_2_MASK) >> SPI_ADDR_2_SHIFT);
	dest[2] = ((address & SPI_ADDR_1_MASK) >> SPI_ADDR_1_SHIFT);
	dest[3] = ((address & SPI_ADDR_0_MASK));
}

void BASpiMemoryDMA::begin(void)
{
	switch (m_memDeviceId) {
	case SpiDeviceId::SPI_DEVICE0 :
		m_csPin = SPI_CS_MEM0;
		m_spi = &SPI;
		m_spi->setMOSI(SPI_MOSI_MEM0);
		m_spi->setMISO(SPI_MISO_MEM0);
		m_spi->setSCK(SPI_SCK_MEM0);
		m_spi->begin();
		m_spiDma = &DMASPI0;
		break;

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
	case SpiDeviceId::SPI_DEVICE1 :
		m_csPin = SPI_CS_MEM1;
		m_spi = &SPI1;
		m_spi->setMOSI(SPI_MOSI_MEM1);
		m_spi->setMISO(SPI_MISO_MEM1);
		m_spi->setSCK(SPI_SCK_MEM1);
		m_spi->begin();
		m_spiDma = &DMASPI1;
		break;
#endif

	default :
		// unreachable since memDeviceId is an enumerated class
		return;
	}

    m_spiDma->begin();
    m_spiDma->start();

    m_started = true;
}

// SPI must build up a payload that starts the teh CMD/Address first. It will cycle
// through the payloads in a circular buffer and use the transfer objects to check if they
// are done before continuing.
void BASpiMemoryDMA::write(size_t address, uint8_t *data, size_t numBytes)
{
	while ( m_txTransfer->busy()) {} // wait until not busy
	uint16_t transferCount = numBytes + 4; // transfer must be increased by the SPI command and address
	m_setSpiCmdAddr(SPI_WRITE_CMD, address, m_txBuffer);
	memcpy(m_txBuffer+4, data, numBytes);
	*m_txTransfer = DmaSpi::Transfer(m_txBuffer, transferCount, nullptr, 0, m_cs);
	m_spiDma->registerTransfer(*m_txTransfer);
}

void BASpiMemoryDMA::zero(size_t address, size_t numBytes)
{
	while ( m_txTransfer->busy()) {}
	uint16_t transferCount = numBytes + 4;
	m_setSpiCmdAddr(SPI_WRITE_CMD, address, m_txBuffer);
	*m_txTransfer = DmaSpi::Transfer(nullptr, transferCount, nullptr, 0, m_cs);
	m_spiDma->registerTransfer(*m_txTransfer);
}

void BASpiMemoryDMA::write16(size_t address, uint16_t *data, size_t numWords)
{
	while ( m_txTransfer->busy()) {}
	size_t numBytes = sizeof(uint16_t)*numWords;
	uint16_t transferCount = numBytes + 4;
	m_setSpiCmdAddr(SPI_WRITE_CMD, address, m_txBuffer);
	memcpy(m_txBuffer+4, data, numBytes);
	*m_txTransfer = DmaSpi::Transfer(m_txBuffer, transferCount, nullptr, 0, m_cs);
	m_spiDma->registerTransfer(*m_txTransfer);
}

void BASpiMemoryDMA::zero16(size_t address, size_t numWords)
{
	while ( m_txTransfer->busy()) {}
	size_t numBytes = sizeof(uint16_t)*numWords;
	uint16_t transferCount = numBytes + 4;
	m_setSpiCmdAddr(SPI_WRITE_CMD, address, m_txBuffer);
	memset(m_txBuffer+4, 0, numBytes);
	*m_txTransfer = DmaSpi::Transfer(m_txBuffer, transferCount, nullptr, 0, m_cs);
	m_spiDma->registerTransfer(*m_txTransfer);
}

void BASpiMemoryDMA::read(size_t address, uint8_t *dest, size_t numBytes)
{
    UNUSED(dest);
	while ( m_rxTransfer->busy()) {}
	uint16_t transferCount = numBytes + 4;
	m_setSpiCmdAddr(SPI_READ_CMD, address, m_rxBuffer);
	*m_rxTransfer = DmaSpi::Transfer(nullptr, transferCount, m_rxBuffer, 0, m_cs);
	m_spiDma->registerTransfer(*m_rxTransfer);
}

void BASpiMemoryDMA::read16(size_t address, uint16_t *dest, size_t numWords)
{
    UNUSED(dest);
	while ( m_rxTransfer->busy()) {}
	m_setSpiCmdAddr(SPI_READ_CMD, address, m_rxBuffer);
	size_t numBytes = sizeof(uint16_t)*numWords;
	uint16_t transferCount = numBytes + 4;
	*m_rxTransfer = DmaSpi::Transfer(nullptr, transferCount, m_rxBuffer, 0, m_cs);
	m_spiDma->registerTransfer(*m_rxTransfer);
}

void BASpiMemoryDMA::readBufferContents(uint8_t *dest, size_t numBytes, size_t bufferOffset)
{
    while (m_rxTransfer->busy()) {} // ensure transfer is complete
	memcpy(dest, m_rxBuffer+4+bufferOffset, numBytes);
}

} /* namespace BAGuitar */
