/**************************************************************************//**
 *  @file
 *  @author Steve Lascos
 *  @company Blackaddr Audio
 *
 *  BASpiMemory is convenience class for accessing the optional SPI RAMs on
 *  the GTA Series boards. BASpiMemoryDma works the same but uses DMA to reduce
 *  load on the processor.
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
#ifndef __BAGUITAR_BASPIMEMORY_H
#define __BAGUITAR_BASPIMEMORY_H

#include <SPI.h>
#include <DmaSpi.h>

#include "BATypes.h"
#include "BAHardware.h"

namespace BAGuitar {

/**************************************************************************//**
 *  This wrapper class uses the Arduino SPI (Wire) library to access the SPI ram.
 *  @details The purpose of this class is primilary for functional testing since
 *  it currently support single-word access. High performance access should be
 *  done using DMA techniques in the Teensy library.
 *****************************************************************************/
class BASpiMemory {
public:
	BASpiMemory() = delete;
	/// Create an object to control either MEM0 (via SPI1) or MEM1 (via SPI2).
	/// @details default is 20 Mhz
	/// @param memDeviceId specify which MEM to control with SpiDeviceId.
	BASpiMemory(SpiDeviceId memDeviceId);
	/// Create an object to control either MEM0 (via SPI1) or MEM1 (via SPI2)
	/// @param memDeviceId specify which MEM to control with SpiDeviceId.
	/// @param speedHz specify the desired speed in Hz.
	BASpiMemory(SpiDeviceId memDeviceId, uint32_t speedHz);
	virtual ~BASpiMemory();

	/// initialize and configure the SPI peripheral
	virtual void begin();

	/// write a single 8-bit word to the specified address
	/// @param address the address in the SPI RAM to write to
	/// @param data the value to write
	void write(size_t address, uint8_t data);

	/// Write a block of 8-bit data to the specified address
	/// @param address the address in the SPI RAM to write to
	/// @param src pointer to the source data block
	/// @param numBytes size of the data block in bytes
	virtual void write(size_t address, uint8_t *src, size_t numBytes);

	/// Write a block of zeros to the specified address
	/// @param address the address in the SPI RAM to write to
	/// @param numBytes size of the data block in bytes
	virtual void zero(size_t address, size_t numBytes);

	/// write a single 16-bit word to the specified address
	/// @param address the address in the SPI RAM to write to
	/// @param data the value to write
	void write16(size_t address, uint16_t data);

	/// Write a block of 16-bit data to the specified address
	/// @param address the address in the SPI RAM to write to
	/// @param src pointer to the source data block
	/// @param numWords size of the data block in 16-bit words
	virtual void write16(size_t address, uint16_t *src, size_t numWords);

	/// Write a block of 16-bit zeros to the specified address
	/// @param address the address in the SPI RAM to write to
	/// @param numWords size of the data block in 16-bit words
	virtual void zero16(size_t address, size_t numWords);

	/// read a single 8-bit data word from the specified address
	/// @param address the address in the SPI RAM to read from
	/// @return the data that was read
	uint8_t read(size_t address);

	/// Read a block of 8-bit data from the specified address
	/// @param address the address in the SPI RAM to write to
	/// @param dest pointer to the destination
	/// @param numBytes size of the data block in bytes
	virtual void read(size_t address, uint8_t *dest, size_t numBytes);

	/// read a single 16-bit data word from the specified address
	/// @param address the address in the SPI RAM to read from
	/// @return the data that was read
	uint16_t read16(size_t address);

	/// read a block 16-bit data word from the specified address
	/// @param address the address in the SPI RAM to read from
	/// @param dest the pointer to the destination
	/// @param numWords the number of 16-bit words to transfer
	virtual void read16(size_t address, uint16_t *dest, size_t numWords);

	/// Check if the class has been configured by a previous begin() call
	/// @returns true if initialized, false if not yet initialized
    bool isStarted() const { return m_started; }

protected:
	SPIClass *m_spi = nullptr;
	SpiDeviceId m_memDeviceId; // the MEM device being control with this instance
	uint8_t m_csPin; // the IO pin number for the CS on the controlled SPI device
	SPISettings m_settings; // the Wire settings for this SPI port
	bool m_started = false;

};


class BASpiMemoryDMA : public BASpiMemory {
public:
	BASpiMemoryDMA() = delete;

	/// Create an object to control either MEM0 (via SPI1) or MEM1 (via SPI2).
	/// @details default is 20 Mhz
	/// @param memDeviceId specify which MEM to control with SpiDeviceId.
	BASpiMemoryDMA(SpiDeviceId memDeviceId);

	/// Create an object to control either MEM0 (via SPI1) or MEM1 (via SPI2)
	/// @param memDeviceId specify which MEM to control with SpiDeviceId.
	/// @param speedHz specify the desired speed in Hz.
	BASpiMemoryDMA(SpiDeviceId memDeviceId, uint32_t speedHz);
	virtual ~BASpiMemoryDMA();

	/// initialize and configure the SPI peripheral
	void begin() override;

	/// Write a block of 8-bit data to the specified address
	/// @param address the address in the SPI RAM to write to
	/// @param src pointer to the source data block
	/// @param numBytes size of the data block in bytes
	void write(size_t address, uint8_t *src, size_t numBytes) override;

	/// Write a block of zeros to the specified address
	/// @param address the address in the SPI RAM to write to
	/// @param numBytes size of the data block in bytes
	void zero(size_t address, size_t numBytes) override;

	/// Write a block of 16-bit data to the specified address
	/// @param address the address in the SPI RAM to write to
	/// @param src pointer to the source data block
	/// @param numWords size of the data block in 16-bit words
	void write16(size_t address, uint16_t *src, size_t numWords) override;

	/// Write a block of 16-bit zeros to the specified address
	/// @param address the address in the SPI RAM to write to
	/// @param numWords size of the data block in 16-bit words
	void zero16(size_t address, size_t numWords) override;

	/// Read a block of 8-bit data from the specified address
	/// @param address the address in the SPI RAM to write to
	/// @param dest pointer to the destination
	/// @param numBytes size of the data block in bytes
	void read(size_t address, uint8_t *dest, size_t numBytes) override;

	/// read a block 16-bit data word from the specified address
	/// @param address the address in the SPI RAM to read from
	/// @param dest the pointer to the destination
	/// @param numWords the number of 16-bit words to transfer
	void read16(size_t address, uint16_t *dest, size_t numWords) override;

	/// Check if a DMA write is in progress
	/// @returns true if a write DMA is in progress, else false
	bool isWriteBusy() const;

	/// Check if a DMA read is in progress
	/// @returns true if a read DMA is in progress, else false
	bool isReadBusy() const;

	/// Readout the 8-bit contents of the DMA storage buffer to the specified destination
	/// @param dest pointer to the destination
	/// @param numBytes number of bytes to read out
	/// @param byteOffset, offset from the start of the DMA buffer in bytes to begin reading
	void readBufferContents(uint8_t *dest,  size_t numBytes, size_t byteOffset = 0);

	/// Readout the 8-bit contents of the DMA storage buffer to the specified destination
	/// @param dest pointer to the destination
	/// @param numWords number of 16-bit words to read out
	/// @param wordOffset, offset from the start of the DMA buffer in words to begin reading
	void readBufferContents(uint16_t *dest, size_t numWords, size_t wordOffset = 0);

private:

	DmaSpiGeneric *m_spiDma = nullptr;
	AbstractChipSelect *m_cs = nullptr;

	uint8_t *m_txCommandBuffer = nullptr;
	DmaSpi::Transfer *m_txTransfer;
	uint8_t *m_rxCommandBuffer = nullptr;
	DmaSpi::Transfer *m_rxTransfer;

	uint16_t m_txXferCount;
	uint16_t m_rxXferCount;

	void m_setSpiCmdAddr(int command, size_t address, uint8_t *dest);
};


} /* namespace BAGuitar */

#endif /* __BAGUITAR_BASPIMEMORY_H */
