/***************************************************************************
 * libgenscd: Gens/GS II CD-ROM Handler Library.                           *
 * ScsiLinux.hpp: Linux SCSI device handler class.                         *
 *                                                                         *
 * Copyright (c) 2013 by David Korth.                                      *
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

#ifndef __LIBGENSCD_SCSILINUX_HPP__
#define __LIBGENSCD_SCSILINUX_HPP__

#ifndef __linux__
#error ScsiLinux is Linux only.
#endif

// ScsiBase class.
#include "ScsiBase.hpp"

namespace LibGensCD
{

class ScsiLinuxPrivate;

class ScsiLinux : public ScsiBase
{
	public:
		ScsiLinux(const std::string& filename);
		virtual ~ScsiLinux();

	private:
		friend class ScsiLinuxPrivate;
		ScsiLinuxPrivate *const d;

		// Q_DISABLE_COPY() equivalent.
		// TODO: Add LibGensCD-specific version of Q_DISABLE_COPY().
		ScsiLinux(const ScsiLinux &);
		ScsiLinux &operator=(const ScsiLinux &);

	public:
		bool isOpen(void) const override final;
		void close(void) override final;

		/**
		 * Print the description of an error code returned by a ScsiBase function.
		 * This may be positive for a SCSI sense key,
		 * negative for an OS error, or 0 for no error.
		 * (No message is printed for 0.)
		 * @param op SCSI operation code.
		 * @param err Error code, as returned by a ScsiBase function.
		 * @param f File handle for fprintf(). (If nullptr, uses stderr.)
		 */
		void printScsiError(uint8_t op, int err, FILE *f = nullptr) override final;

		/**
		 * Check if a disc is present.
		 * @return True if a disc is present; false if not.
		 */
		bool isDiscPresent(void) override final;

		/**
		 * Check if the disc has changed since the last access.
		 * @return True if the disc has changed; false if not.
		 */
		bool hasDiscChanged(void) override final;

	protected:
		/**
		 * Send a SCSI command descriptor block to the drive.
		 * @param cdb		[in] SCSI command descriptor block.
		 * @param cdb_len	[in] Length of cdb.
		 * @param out		[out] Output buffer, or nullptr if no data is requested.
		 * @param out_len	[out] Length of out.
		 * @param mode		[in] Data direction mode. (IN == receive from device; OUT == send to device)
		 * @return 0 on success, positive for SCSI sense key, negative for OS error.
		 */
		int scsi_send_cdb(const void *cdb, uint8_t cdb_len,
				  void *out, size_t out_len,
				  scsi_data_mode mode = SCSI_DATA_IN) override final;
};

}

#endif /* __LIBGENSCD_SCSILINUX_HPP__ */
