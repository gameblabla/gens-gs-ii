/***************************************************************************
 * libgens: Gens Emulation Library.                                        *
 * EEPRomI2C.cpp: I2C Serial EEPROM handler.                               *
 *                                                                         *
 * Copyright (c) 2015 by David Korth.                                      *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

/**
 * Based on cart_hw/eeprom.c from Genesis Plus GX.
 */

#include "EEPRomI2C.hpp"

#include "macros/common.h"
#include "macros/log_msg.h"

// C includes (C++ namespace).
#include <cstring>

#include "EEPRomI2C_p.hpp"
namespace LibGens {

/** EEPRomI2CPrivate **/

EEPRomI2CPrivate::EEPRomI2CPrivate(EEPRomI2C *q)
	: q(q)
	, dirty(false)
	, framesElapsed(0)
{
	// Clear the EEPRom type.
	memset(&eprSpec, 0, sizeof(eprSpec));
	memset(&eprMapper, 0, sizeof(eprMapper));

	// Reset the EEPRom.
	reset();
}

/**
 * Clear the EEPRom and initialize settings.
 * This function does NOT reset the EEPRom type!
 */
void EEPRomI2CPrivate::reset(void)
{
	// EEProm is initialized with 0xFF.
	memset(eeprom, 0xFF, sizeof(eeprom));
	clearDirty();

	// Reset the clock and data line states.
	scl = 1;
	sda_out = 1;
	sda_in = 1;
	scl_prev = 1;
	sda_out_prev = 1;
	sda_in_prev = 1;

	// Reset the internal registers.
	address = 0;
	counter = 0;
	rw = 0;

	// Shift register.
	shift_rw = 0,	// Shift-in by default.
	data_buf = 0;

	// Reset the state.
	state = EPR_STANDBY;
}

/**
 * Process an I2C bit.
 */
void EEPRomI2CPrivate::processI2Cbit(void)
{
	// Save the current /SDA out.
	sda_out_prev = sda_out;

	if (eprMapper.epr_type == EEPRomI2C::EPR_NONE) {
		// No EEPRom.
		goto done;
	}

	// Check for a STOP condition.
	if (checkStop()) {
		// STOP condition reached.
		LOG_MSG(eeprom_i2c, LOG_MSG_LEVEL_DEBUG1,
			"Received STOP condition.");
		counter = 0;
		sda_out = 1;
		data_buf = 0;
		shift_rw = 0;	// shift in
		state = EPR_STANDBY;
		goto done;
	}

	// Has a START condition been issued?
	if (checkStart()) {
		// START condition.
		LOG_MSG(eeprom_i2c, LOG_MSG_LEVEL_DEBUG1,
			"Received START condition.");
		counter = 0;
		sda_out = 1;
		data_buf = 0;
		shift_rw = 0;	// shift in
		if (eprMapper.epr_type == EEPRomI2C::EPR_X24C01) {
			// Mode 1.
			state = EPR_MODE1_WORD_ADDRESS;
		} else {
			// Mode 2 or 3.
			// TODO
			//state = EPR_MODE2_WORD_ADDRESS;
		}
		goto done;
	}

	if (state == EPR_STANDBY) {
		// Nothing to do right now.
		goto done;
	}

	// Check if we're shifting in or shifting out.
	if (!shift_rw) {
		// Shifting in.
		// Master device is writing an 8-bit data word.

		// Check for SCL low-to-high.
		if (checkSCL_LtoH()) {
			if (counter >= 8) {
				// Finished receiving the data word.
				// Acknowledge it.
				sda_out = 0;
				counter++;

				// Determine what to do based on the current state.
				// TODO: Split into a separate function?
				switch (state) {
					case EPR_MODE1_WORD_ADDRESS:
						// X24C01 word address.
						address = (data_buf >> 1) & 0x7F;
						rw = (data_buf & 1);
						counter = 0;
						if (rw) {
							// Read data.
							data_buf = eeprom[address];
							shift_rw = 1;	// Shifting out.
							state = EPR_READ_DATA;
						} else {
							// Write data.
							data_buf = 0;
							shift_rw = 0;	// Shifting in.
							state = EPR_WRITE_DATA;
						}
						LOG_MSG(eeprom_i2c, LOG_MSG_LEVEL_DEBUG1,
							"EPR_MODE1_WORD_ADDRESS: address=%02X, rw=%d, data_buf=%02X",
							address, rw, data_buf);
						break;

					case EPR_WRITE_DATA: {
						// Save the data byte.
						eeprom[address] = data_buf;
						setDirty();

						// Next byte in the page.
						uint16_t prev_address = address;
						uint16_t addr_low_tmp = address;
						addr_low_tmp++;
						addr_low_tmp &= eprSpec.pg_mask;
						address = ((address & ~eprSpec.pg_mask) | addr_low_tmp);

						data_buf = 0;
						LOG_MSG(eeprom_i2c, LOG_MSG_LEVEL_DEBUG1,
							"EPR_WRITE_DATA: %02X -> [%02X]; address=%02X",
							eeprom[prev_address], prev_address, address);
						break;
					}

					default:
						// Unknown.
						// Go back to standby.
						sda_out = 1;
						counter = 0;
						data_buf = 0;
						shift_rw = 0;
						state = EPR_STANDBY;
				}
			} else {
				// Data bit is valid.
				data_buf <<= 1;
				data_buf |= getSDA();
				counter++;
			}
		} else if (checkSCL_HtoL()) {
			// Release the data line.
			sda_out = 1;
			if (counter >= 9) {
				// Acknowledged.
				// Reset the counter.
				counter = 0;
			}
		}
	} else {
		// Shifting out.
		// Master device is reading an 8-bit data word.
		// NOTE: This is usually EPR_READ_DATA.

		// Check for SCL high-to-low.
		if (checkSCL_LtoH()) {
			if (counter >= 9) {
				// Is this an acknowlege?
				if (!getSDA()) {
					// Acknowledged by master.

					// Determine what to do based on the current state.
					// TODO: Split into a separate function?
					switch (state) {
						case EPR_READ_DATA:
							// Go to the next byte.
							// NOTE: Page mask does NOT apply to reads.
							address++;
							address &= eprSpec.sz_mask;
							data_buf = eeprom[address];
							counter = 0;
							LOG_MSG(eeprom_i2c, LOG_MSG_LEVEL_DEBUG1,
								"EPR_READ_DATA: ACK received: address=%02X, data_buf=%02X",
								address, data_buf);
							break;

						default:
							// Unknown.
							// Go back to standby.
							sda_out = 1;
							counter = 0;
							data_buf = 0;
							shift_rw = 0;
							state = EPR_STANDBY;
					}
				}
			}
		} else if (checkSCL_HtoL()) {
			if (counter < 8) {
				// Send a data bit to the host.
				// NOTE: MSB is out first.
				sda_out = !!(data_buf & 0x80);
				data_buf <<= 1;
				counter++;
			} else if (counter == 8) {
				// Release the data line.
				sda_out = 1;
				counter++;
			}

			// TODO: Not necessarily EPR_READ_DATA.
			if (counter == 8) {
				LOG_MSG(eeprom_i2c, LOG_MSG_LEVEL_DEBUG1,
					"EPR_READ_DATA: all 8 bits read: address=%02X",
					address);
			}
		}
	}

done:
	// Save the current /SCL and /SDA.
	scl_prev = scl;
	sda_in_prev = sda_in;
}

/** EEPRom **/

/**
 * Initialize the EEPRom chip.
 */
EEPRomI2C::EEPRomI2C()
	: d(new EEPRomI2CPrivate(this))
{ }

EEPRomI2C::~EEPRomI2C()
{
	delete d;
}

/**
 * Clear the EEPRom and initialize settings.
 * This function does NOT reset the EEPRom type!
 */
void EEPRomI2C::reset(void)
{
	d->reset();
}

/**
 * Set the EEPRom type.
 * @param type EEPRom type. (Specify a negative number to clear)
 * @return 0 on success; non-zero on error.
 */
int EEPRomI2C::setEEPRomType(int type)
{
	if (type < 0) {
		// Negative type ID. Reset the EEPRom.
		d->reset();
		return 0;
	} else if (type >= ARRAY_SIZE(d->rom_db)) {
		// Type ID is out of range.
		return 1;
	}

	// Set the EEPRom type.
	memcpy(&d->eprMapper, &d->rom_db[type].mapper, sizeof(d->eprMapper));
	memcpy(&d->eprSpec, &d->eeprom_spec[d->eprMapper.epr_type], sizeof(d->eprSpec));
	return 0;
}

/**
 * Determine if the EEPRom type is set.
 * @return True if the EEPRom type is set; false if not.
 */
bool EEPRomI2C::isEEPRomTypeSet(void) const
{
	//printf("sz mask: 0x%02X\n", d->eprSpec.sz_mask);
	return !(d->eprSpec.sz_mask == 0);
}

/**
 * Address verification functions.
 *
 * Notes:
 *
 * - Address 0 doesn't need to be checked, since the M68K memory handler
 *   never checks EEPROM in the first bank (0x000000 - 0x07FFFF).
 *
 * - Word-wide addresses are checked by OR'ing both the specified address
 *   and the preset address with 1.
 *
 * @param address Address.
 * @return True if the address is usable for the specified purpose.
 */

bool EEPRomI2C::isReadBytePort(uint32_t address) const
{
	return (address == d->eprMapper.sda_out_adr);
}

bool EEPRomI2C::isReadWordPort(uint32_t address) const
{
	return ((address | 1) == (d->eprMapper.sda_out_adr | 1));
}

bool EEPRomI2C::isWriteBytePort(uint32_t address) const
{
	return (address == d->eprMapper.scl_adr ||
		address == d->eprMapper.sda_in_adr);
}

bool EEPRomI2C::isWriteWordPort(uint32_t address) const
{
	address |= 1;
	return ((address == (d->eprMapper.scl_adr | 1)) ||
		(address == (d->eprMapper.sda_in_adr | 1)));
}

/**
 * Check if the EEPRom is dirty.
 * @return True if EEPRom has been modified since the last save; false otherwise.
 */
bool EEPRomI2C::isDirty(void) const
	{ return d->isDirty(); }

/**
 * Read the specified port. (byte-wide)
 * @param address Address.
 * @return Port value.
 */
uint8_t EEPRomI2C::readByte(uint32_t address)
{
	if (address != d->eprMapper.sda_out_adr) {
		// Wrong address.
		return 0xFF;
	}

	// Return /SDA, shifted over to the appropriate position.
	// TODO: Other bits should be prefetch?
	return (d->getSDA() << d->eprMapper.sda_out_bit);
}


/**
 * Read the specified port. (word-wide)
 * @param address Address.
 * @return Port value.
 */
uint16_t EEPRomI2C::readWord(uint32_t address)
{
	// TODO: address probably doesn't need to be masked,
	// since M68K is word-aligned...
	if ((address & ~1) != (d->eprMapper.sda_out_adr & ~1)) {
		// Wrong address.
		return 0xFFFF;
	}

	// Return /SDA, shifted over to the appropriate position.
	// TODO: Other bits should be prefetch?
	uint8_t sda_out_bit = d->eprMapper.sda_out_bit;
	sda_out_bit += ((d->eprMapper.sda_out_adr & 1) * 8);
	return (d->getSDA() << sda_out_bit);
}

/**
 * Write to the specified port. (byte-wide)
 * @param address Address.
 * @param data Data.
 */
void EEPRomI2C::writeByte(uint32_t address, uint8_t data)
{
	if (address != d->eprMapper.scl_adr &&
	    address != d->eprMapper.sda_in_adr)
	{
		// Invalid address.
		return;
	}

	// Check if this is the clock line. (/SCL)
	if (address == d->eprMapper.scl_adr) {
		d->scl = (data >> (d->eprMapper.scl_bit)) & 1;
	} else {
		d->scl = d->scl_prev;
	}

	// Check if this is the data line. (/SDA)
	if (address == d->eprMapper.sda_in_adr) {
		d->sda_in = (data >> (d->eprMapper.sda_in_bit)) & 1;
	} else {
		d->sda_in = d->sda_in_prev;
	}

	// Process the I2C command.
	// TODO: Only if /SDA or /SCL has changed?
	d->processI2Cbit();
}

/**
 * Write to the specified port. (word-wide)
 * @param address Address.
 * @param data Data.
 */
void EEPRomI2C::writeWord(uint32_t address, uint16_t data)
{
	// TODO: Verify that the address is valid.

	// Mask off the address LSB.
	address &= ~1;

	// Check if this is the clock line. (/SCL)
	if (address == d->eprMapper.scl_adr) {
		d->scl = (data >> (d->eprMapper.scl_bit + 8)) & 1;
	} else if ((address | 1) == d->eprMapper.scl_adr) {
		d->scl = (data >> (d->eprMapper.scl_bit)) & 1;
	} else {
		d->scl = d->scl_prev;
	}

	// Check if this is the data line. (/SDA)
	if (address == d->eprMapper.sda_in_adr) {
		d->sda_in = (data >> (d->eprMapper.sda_in_bit + 8)) & 1;
	} else if ((address | 1) == d->eprMapper.sda_in_adr) {
		d->sda_in = (data >> (d->eprMapper.sda_in_bit)) & 1;
	} else {
		d->sda_in = d->sda_in_prev;
	}

	// Process the I2C bit.
	// TODO: Only if /SDA or /SCL has changed?
	d->processI2Cbit();
}

}
