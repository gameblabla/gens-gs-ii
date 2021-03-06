/***************************************************************************
 * libgens: Gens Emulation Library.                                        *
 * Device.hpp: Base I/O device.                                            *
 *                                                                         *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville.                      *
 * Copyright (c) 2003-2004 by Stéphane Akhoun.                             *
 * Copyright (c) 2008-2015 by David Korth.                                 *
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

#ifndef __LIBGENS_IO_DEVICE_HPP__
#define __LIBGENS_IO_DEVICE_HPP__

#include "IoManager.hpp"

namespace LibGens { namespace IO {

class Device
{
	public:
		Device();
		virtual ~Device();
		// TODO: Add constructor that takes another IoDevice
		// in order to copy ctrl/etc.
		// Do this for subclasses, too.

	private:
		// Q_DISABLE_COPY() equivalent.
		// TODO: Add LibGens-specific version of Q_DISABLE_COPY().
		Device(const Device &);
		Device &operator=(const Device &);

	public:
		/**
		 * Completely reset the Device.
		 * This should be done on emulation startup or Hard Reset only.
		 */
		void reset(void);

		/**
		 * Reset Device data that only affects the device
		 * and not the emulation-side registers.
		 *
		 * Should be overridden by subclasses that have
		 * device-specific data.
		 */
		virtual void resetDev(void);

		/**
		 * Device type.
		 * Subclasses must set m_type in their constructor.
		 * @return Device type.
		 */
		IoManager::IoType_t type(void) const;

		/**
		 * Does this device have a standard D-Pad as buttons 0-3?
		 *
		 * This is used by IoManager to determine if the D-Pad
		 * buttons should be constrained such that the user can't
		 * press both U+D or L+R at the same time.
		 *
		 * The default is true. Devices that don't have a standard
		 * D-Pad should set m_hasDPad = false in their constructor.
		 *
		 * @return True if this device has a standard D-Pad.
		 */
		bool hasDPad(void) const;

		// Device-side variables.
		int counter;			// Internal counter.
		uint8_t deviceData;		// Data written from the device.

		// System-side variables.
		uint8_t ctrl;			// Tristate control.
		uint8_t mdData;			// Data written from the MD.

		// Tristate cache for data from MD.
		uint8_t mdData_tris;		// MD data cache.

		// I/O pin definitions.
		enum IoPin_t {
			IOPIN_UP	= 0x01,	// D0
			IOPIN_DOWN	= 0x02,	// D1
			IOPIN_LEFT	= 0x04,	// D2
			IOPIN_RIGHT	= 0x08,	// D3
			IOPIN_TL	= 0x10,	// D4
			IOPIN_TR	= 0x20,	// D5
			IOPIN_TH	= 0x40	// D6
		};

	protected:
		/**
		 * Device type.
		 * Subclasses must set this field in their constructor.
		 */
		IoManager::IoType_t m_type;

		// Does this device have a standard D-Pad as buttons 0-3?
		bool m_hasDPad;

		/**
		 * Controller bitfield.
		 * Format:
		 * - 2-button:          ??CBRLDU
		 * - 3-button:          SACBRLDU
		 * - 6-button: ????MXYZ SACBRLDU
		 * NOTE: ACTIVE LOW! (1 == released; 0 == pressed)
		 */
		uint32_t buttons;

		// Previous buttons value.
		uint32_t buttons_prev;

		/**
		 * Update the tristate cache for data coming from the MD.
		 * NOTE: "In" == MD to controller; "Out" == controller to MD.
		 */
		inline void updateTristateInputCache(void) {
			// TODO: Apply the device data?
			mdData_tris = (~ctrl | mdData);
		}

		/**
		 * Absolute positioning.
		 * Ranges:
		 * - X: [0, 1279]
		 * - Y: [0, 239] or [0, 479]
		 * "Offscreen" is represented by abs_x = abs_y = -1.
		 */
		int m_abs_x, m_abs_y;

	public:
		/**
		 * Get the buttons bitfield.
		 * Used by some multitaps.
		 * @return Buttons bitfield.
		 */
		inline uint32_t getButtons(void) const {
			return this->buttons;
		}

		/**
		 * Check an input line's state.
		 * @param ioPin I/O pin, or multiple pins.
		 * @return 0 if low; non-zero if high.
		 */
		inline uint8_t checkInputLine(IoPin_t ioPin) const
		{
			return (mdData_tris & ioPin);
		}

		// TODO: Consolidate this stuff so it can be cached properly.

		/**
		 * Read the last output data value, with tristates applied.
		 * @return Data value with tristate settings applied.
		 */
		inline uint8_t readData(void) const {
			return applyTristate(deviceData);
		}

		/**
		 * Apply the Tristate settings to the data value.
		 * @param data Data value.
		 * @return Data value with tristate settings applied.
		 */
		inline uint8_t applyTristate(uint8_t data) const {
			data &= (~ctrl & 0x7F);		// Mask output bits.
			data |= (mdData & (ctrl | 0x80));	// Apply data buffer.
			return data;
		}

		// Serial I/O variables.
		// TODO: Serial data buffer.
		uint8_t serCtrl;	// Serial control.
		uint8_t serLastTx;	// Last transmitted data byte.

		/**
		 * Update the I/O device.
		 * Saves the new buttons, then runs the internal device update.
		 * @param buttons New button state.
		 */
		void update(uint32_t buttons);

		/**
		 * Update the I/O device's absolute tablet coordinates.
		 * Coordinates must be scaled to 1280x240.
		 * If the input device is offscreen, both X and Y should be -1.
		 * For two-display devices, Y should be [0,239] for the top screen,
		 * and [240,479] for the bottom screen.
		 * @param x X coordinate.
		 * @param y Y coordinate.
		 */
		void updateAbsolutePosition(int x, int y);

		/**
		 * Update the I/O device.
		 * Runs the internal device update.
		 */
		virtual void update(void);

		/**
		 * One scanline worth of time has passed.
		 * Needed for some devices that reset after a period of time,
		 * e.g. 6BTN controllers.
		 */
		virtual void update_onScanline(void);

		/**
		 * Device port was read.
		 * Only applies to devices on physical ports.
		 * Needed for some devices that have partially-unclocked protocols.
		 */
		virtual void update_onRead(void);

		/**
		 * Set a sub-device.
		 * Used for multitaps. (Base implementation does nothing.)
		 * @param virtPort Virtual port number.
		 * @param ioDevice I/O device.
		 * @return 0 on success; non-zero on error.
		 */
		virtual int setSubDevice(int virtPort, Device *ioDevice);

		/**
		 * Set the Pin 5/8 status.
		 * NOTE: Only the low two bits are saved.
		 * @param pin58 New pin 5/8 status.
		 */
		void setPin58(uint8_t pin58);

		/**
		 * Get the Pin 5/8 status.
		 * @return Pin 5/8 status.
		 */
		uint8_t pin58(void);

	protected:
		/**
		 * Pin 5/8 status.
		 * Pin 5 is usually +5V; pin 8 is usually ground.
		 * ColecoVision uses this to switch between joystick and keypad.
		 * Other systems should not use this at all.
		 *
		 * Bit 0 == pin 8 (low for joystick)
		 * Bit 1 == pin 5 (low for keypad)
		 */
		uint8_t m_pin58;
};

} }

#endif /* __LIBGENS_IO_DEVICE_HPP__ */
