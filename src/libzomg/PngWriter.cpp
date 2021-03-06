/***************************************************************************
 * libzomg: Zipped Original Memory from Genesis.                           *
 * PngWriter.cpp: PNG image writer.                                        *
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

#include "PngWriter.hpp"
#include "img_data.h"

// libpng
// MUST be included before Metadata.hpp.
#include <png.h>

// ZOMG metadata.
#include "Metadata.hpp"

// MiniZip
#include "minizip/zip.h"

#ifdef _WIN32
// Win32 Unicode Translation Layer.
#include "libcompat/W32U/W32U_mini.h"
#endif

// C includes.
#include <stdint.h>
#include <stdlib.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

// C includes. (C++ namespace)
#include <cerrno>
#include <csetjmp>
#include <cstring>

// C++ includes.
#include <string>
#include <vector>
using std::string;
using std::vector;

// libpng-1.5.0 marked several function parameters as const.
// Older versions don't have them marked as const, but they're
// effectively const anyway.
#if PNG_LIBPNG_VER_MAJOR > 1 || (PNG_LIBPNG_VER_MAJOR == 1 && PNG_LIBPNG_VER_MINOR >= 5)
// libpng-1.5.0: Functions have const pointer arguments.
#define PNG_CONST_CAST(type, ptr) (ptr)
#else
// libpng-1.4.x or earlier: Functions do *not* have const pointer arguments.
#define PNG_CONST_CAST(type, ptr) const_cast<type>(ptr)
#endif

namespace LibZomg {

// TODO: Convert to a static class?

/** PngWriterPrivate **/

class PngWriterPrivate
{
	public:
		PngWriterPrivate(PngWriter *q);
		~PngWriterPrivate();

	protected:
		friend class PngWriter;
		PngWriter *const q;
	private:
		// Q_DISABLE_COPY() equivalent.
		// TODO: Add LibPngWriter-specific version of Q_DISABLE_COPY().
		PngWriterPrivate(const PngWriterPrivate &);
		PngWriterPrivate &operator=(const PngWriterPrivate &);

	public:
		/**
		 * Write 16-bit PNG rows.
		 * @param pixel Typename.
		 * @param RBits Red bits.
		 * @param GBits Green bits.
		 * @param BBits Blue bits.
		 * @param img_data Image data.
		 * @param row_buffer Row buffer. (Must be at least width * 3 bytes.)
		 * @param png_ptr PNG pointer.
		 */
		template<typename pixel, uint8_t RBits, uint8_t GBits, uint8_t BBits>
		static void T_writePNG_rows_16(const Zomg_Img_Data_t *img_data, uint8_t *row_buffer,
					       png_structp png_ptr);

	public:
		/**
		 * PNG MiniZip write function.
		 * @param png_ptr PNG pointer.
		 * @param buf Data to write.
		 * @param len Size of buf.
		 */
		static void png_io_minizip_write(png_structp png_ptr, png_bytep buf, png_size_t len);

		/**
		 * PNG MiniZip flush function.
		 * Required when writing PNG images.
		 * This implementation is a no-op.
		 * @param png_ptr PNG pointer.
		 */
		static void png_io_minizip_flush(png_structp png_ptr);

		/**
		 * Internal PNG write function.
		 * @param png_ptr	[in] PNG pointer.
		 * @param info_ptr	[in] PNG info pointer.
		 * @param img_data	[in] PNG image data.
		 * @param metadata	[in, opt] Extra metadata.
		 * @param metaFlags	[in, opt] Metadata flags.
		 * @return 0 on success; negative errno on error.
		 */
		static int writeToPng(png_structp png_ptr, png_infop info_ptr,
				      const Zomg_Img_Data_t *img_data,
				      const Metadata *metadata, int metaFlags);
};

PngWriterPrivate::PngWriterPrivate(PngWriter *q)
	: q(q)
{ }

PngWriterPrivate::~PngWriterPrivate()
{ }

/**
 * Write 16-bit PNG rows.
 * @param pixel Typename.
 * @param RBits Red bits.
 * @param GBits Green bits.
 * @param BBits Blue bits.
 * @param img_data Image data.
 * @param row_buffer Row buffer. (Must be at least width * 3 bytes.)
 * @param png_ptr PNG pointer.
 */
template<typename pixel, uint8_t RBits, uint8_t GBits, uint8_t BBits>
void PngWriterPrivate::T_writePNG_rows_16(const Zomg_Img_Data_t *img_data, uint8_t *row_buffer,
					  png_structp png_ptr)
{
	#define MMASK(bits) ((1 << (bits)) - 1)
	uint8_t r, g, b;

	// Write the rows.
	const uint16_t *screen = (const uint16_t*)img_data->data;
	const int row_adj = (img_data->pitch / sizeof(pixel)) - img_data->w;
	for (int y = img_data->h; y > 0; y--) {
		uint8_t *rowBufPtr = row_buffer;
		for (int x = img_data->w; x > 0; x--, rowBufPtr += 3, screen++) {
			// Get the color components.
			r = (uint8_t)((*screen >> (GBits + BBits)) & MMASK(RBits)) << (8 - RBits);
			g = (uint8_t)((*screen >> BBits) & MMASK(GBits)) << (8 - GBits);
			b = (uint8_t)((*screen) & MMASK(BBits)) << (8 - BBits);

			// Fill in the unused bits with a copy of the MSBs.
			r |= (r >> RBits);
			g |= (g >> GBits);
			b |= (b >> BBits);

			// Save the new color components.
			*(rowBufPtr + 0) = r;
			*(rowBufPtr + 1) = g;
			*(rowBufPtr + 2) = b;
		}

		// Write the row.
		png_write_row(png_ptr, row_buffer);

		// Next row.
		screen += row_adj;
	}
}

/**
 * PNG MiniZip write function.
 * @param png_ptr PNG pointer.
 * @param buf Data to write.
 * @param len Size of buf.
 */
void PngWriterPrivate::png_io_minizip_write(png_structp png_ptr, png_bytep buf, png_size_t len)
{
	// Assuming io_ptr is a zipFile.
	zipFile zf = reinterpret_cast<zipFile>(png_get_io_ptr(png_ptr));
	if (!zf)
		return;

	// TODO: Check the return value!
	zipWriteInFileInZip(zf, buf, len);
}

/**
 * PNG MiniZip flush function.
 * Required when writing PNG images.
 * This implementation is a no-op.
 * @param png_ptr PNG pointer.
 */
void PngWriterPrivate::png_io_minizip_flush(png_structp png_ptr)
{
        // Do nothing!
        ((void)png_ptr);
}

/**
 * Internal PNG write function.
 * @param png_ptr	[in] PNG pointer.
 * @param info_ptr	[in] PNG info pointer.
 * @param img_data	[in] PNG image data.
 * @param metadata	[in, opt] Extra metadata.
 * @param metaFlags	[in, opt] Metadata flags.
 * @return 0 on success; negative errno on error.
 */
int PngWriterPrivate::writeToPng(png_structp png_ptr, png_infop info_ptr,
				 const Zomg_Img_Data_t *img_data,
				 const Metadata *metadata, int metaFlags)
{
	// Row pointers and/or buffer.
	// These need to be allocated here so they can be freed
	// in case an error occurs.
	// FIXME: gcc-5.2 is showing this warning:
	// warning: variable ‘row’ might be clobbered by ‘longjmp’ or ‘vfork’ [-Wclobbered]
	union {
		void *p;

		// Row pointers. (32-bit color)
		// Each entry points to the beginning of a row.
		png_byte **row_pointers;

		// Row buffer. (15-bit or 16-bit color)
		// libpng doesn't support 15-bit or 16-bit color natively,
		// so the rows have to be converted.
		png_byte *row_buffer;
	} row;

	if (img_data->bpp == 32) {
		row.row_pointers = (png_byte**)png_malloc(png_ptr, sizeof(png_byte*) * img_data->h);
	} else {
		row.row_buffer = (png_byte*)png_malloc(png_ptr, sizeof(png_byte*) * img_data->w * 3);
	}
	if (!row.p) {
		// Not enough memory is available.
		return -ENOMEM;
	}

	// WARNING: Do NOT initialize any C++ objects past this point!
#ifdef PNG_SETJMP_SUPPORTED
	if (setjmp(png_jmpbuf(png_ptr))) {
		// PNG write failed.
		png_free(png_ptr, row.p);
		// TODO: Better error code?
		return -ENOMEM;
	}
#endif /* PNG_SETJMP_SUPPORTED */

	// Disable PNG filters.
	png_set_filter(png_ptr, 0, PNG_FILTER_NONE);

	// Set the compression level to 5. (Levels range from 1 to 9.)
	// TODO: Add a parameter for this?
	png_set_compression_level(png_ptr, 5);

	// Set up the PNG header.
	png_set_IHDR(png_ptr, info_ptr, img_data->w, img_data->h,
		     8,				// Color depth (per channel).
		     PNG_COLOR_TYPE_RGB,	// RGB color.
		     PNG_INTERLACE_NONE,	// No interlacing.
		     PNG_COMPRESSION_TYPE_DEFAULT,
		     PNG_FILTER_TYPE_DEFAULT
		     );

	// Write the sBIT chunk.
	static const png_color_8 sBIT_15 = {5, 5, 5, 0, 0};
	static const png_color_8 sBIT_16 = {5, 6, 5, 0, 0};
	static const png_color_8 sBIT_32 = {8, 8, 8, 0, 0};
	switch (img_data->bpp) {
		case 15:
			png_set_sBIT(png_ptr, info_ptr, PNG_CONST_CAST(png_color_8*, &sBIT_15));
			break;
		case 16:
			png_set_sBIT(png_ptr, info_ptr, PNG_CONST_CAST(png_color_8*, &sBIT_16));
			break;
		case 32:
		default:
			png_set_sBIT(png_ptr, info_ptr, PNG_CONST_CAST(png_color_8*, &sBIT_32));
			break;
	}

	// Write the pHYs chunk.
	if (img_data->phys_x > 0 && img_data->phys_y > 0) {
		png_set_pHYs(png_ptr, info_ptr,
			     img_data->phys_x, img_data->phys_y,
			     PNG_RESOLUTION_UNKNOWN);
	}

	// PNG metadata.
	if (metadata) {
		metadata->toPngData(png_ptr, info_ptr, metaFlags);
	} else {
		// No metadata specified.
		// Use a blank Metadata object, which is basically CreationTime only.
		Metadata metadata_default;
		metadata_default.toPngData(png_ptr, info_ptr, metaFlags);
	}

	// Write the PNG header to the file.
	png_write_info(png_ptr, info_ptr);

	// Write the image.
	switch (img_data->bpp) {
		case 15: {
			// 15-bit color. (555)
			T_writePNG_rows_16<uint16_t, 5, 5, 5>
					  (img_data, row.row_buffer, png_ptr);
			break;
		}

		case 16: {
			// 16-bit color. (565)
			T_writePNG_rows_16<uint16_t, 5, 6, 5>
					  (img_data, row.row_buffer, png_ptr);
			// TODO
			break;
		}

		case 32:
		default:
			// Initialize the row pointers array.
			// TODO: const uint8_t*.
			uint8_t *data = (uint8_t*)img_data->data + ((img_data->h - 1) * img_data->pitch);
			for (int y = img_data->h - 1; y >= 0; y--, data -= img_data->pitch) {
				row.row_pointers[y] = data;
			}

			// libpng expects RGB data with no alpha channel, i.e. 24-bit.
			// However, there is an option to automatically convert 32-bit
			// without alpha channel to 24-bit, so we'll use that.
			png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);

			// We're using "BGR" color.
			png_set_bgr(png_ptr);

			// Write the image.
			png_write_image(png_ptr, row.row_pointers);
			break;
	}

	// Free the row pointers.
	png_free(png_ptr, row.p);

	// Finished writing the PNG image.
	png_write_end(png_ptr, info_ptr);
	return 0;
}

/** PngWriter **/

PngWriter::PngWriter()
	: d(new PngWriterPrivate(this))
{ }

PngWriter::~PngWriter()
{
	delete d;
}

/**
 * Write an image to a PNG file.
 * No metadata other than creation time will be saved.
 * @param img_data	[in] Image data.
 * @param filename	[in] PNG file.
 * @return 0 on success; negative errno on error.
 */
int PngWriter::writeToFile(const Zomg_Img_Data_t *img_data,
			   const char *filename)
{
	return writeToFile(img_data, filename, nullptr, Metadata::MF_Default);
}

/**
 * Write an image to a PNG file.
 * @param img_data	[in] Image data.
 * @param filename	[in] PNG file.
 * @param metadata	[in, opt] Extra metadata.
 * @param metaFlags	[in, opt] Metadata flags.
 * @return 0 on success; negative errno on error.
 */
int PngWriter::writeToFile(const Zomg_Img_Data_t *img_data,
			   const char *filename,
			   const Metadata *metadata,
			   int metaFlags)
{
	// TODO: Combine more of writeToFile() and writeToZip().
	if (!filename || !img_data || !img_data->data ||
	    img_data->w <= 0 || img_data->h <= 0 ||
	    (img_data->bpp != 15 && img_data->bpp != 16 && img_data->bpp != 32)) {
		// Invalid parameters.
		return -EINVAL;
	}

	// Set some sane limits for image size.
	if (img_data->w > 16384 || img_data->h > 16384) {
		// Image is too big.
		// TODO: -ENOSPC?
		return -ENOMEM;
	}

	// Calculate the minimum pitch.
	const unsigned int min_pitch = img_data->w * (img_data->bpp == 32 ? 4 : 2);
	if (img_data->pitch < min_pitch) {
		// Invalid parameters.
		return -EINVAL;
	}

	png_structp png_ptr;
	png_infop info_ptr;

	// Initialize libpng.
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png_ptr) {
		return -ENOMEM;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, nullptr);
		return -ENOMEM;
	}

	// Output file.
	FILE *f = fopen(filename, "wb");
	if (!f) {
		// Error opening the output file.
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return -errno;
	}

	// Initialize standard file I/O.
	png_init_io(png_ptr, f);

	// Write to PNG.
	int ret = d->writeToPng(png_ptr, info_ptr, img_data, metadata, metaFlags);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(f);

	if (ret != 0) {
		// Failed to write the PNG file.
		// Delete it so we don't end up with a half-written image.
		unlink(filename);
	}

	return ret;
}

/**
 * Write an image to a PNG file in a ZIP file.
 * No metadata other than creation time will be saved.
 * @param img_data	[in] Image data.
 * @param zfile		[in] ZIP file. (Must have a file open for writing.)
 * @return 0 on success; negative errno on error.
 */
int PngWriter::writeToZip(const Zomg_Img_Data_t *img_data, zipFile zfile)
{
	return writeToZip(img_data, zfile, nullptr, Metadata::MF_Default);
}

/**
 * Write an image to a PNG file in a ZIP file.
 * @param img_data	[in] Image data.
 * @param zfile		[in] ZIP file. (Must have a file open for writing.)
 * @param metadata	[in, opt] Extra metadata.
 * @param metaFlags	[in, opt] Metadata flags.
 * @return 0 on success; negative errno on error.
 */
int PngWriter::writeToZip(const Zomg_Img_Data_t *img_data,
			  zipFile zfile,
			  const Metadata *metadata,
			  int metaFlags)
{
	// TODO: Combine more of writeToFile() and writeToZip().
	if (!zfile || !img_data || !img_data->data ||
	    img_data->w <= 0 || img_data->h <= 0 ||
	    (img_data->bpp != 15 && img_data->bpp != 16 && img_data->bpp != 32)) {
		// Invalid parameters.
		return -EINVAL;
	}

	// Set some sane limits for image size.
	if (img_data->w > 16384 || img_data->h > 16384) {
		// Image is too big.
		// TODO: -ENOSPC?
		return -ENOMEM;
	}

	// Calculate the minimum pitch.
	const unsigned int min_pitch = img_data->w * (img_data->bpp == 32 ? 4 : 2);
	if (img_data->pitch < min_pitch) {
		// Invalid parameters.
		return -EINVAL;
	}

	png_structp png_ptr;
	png_infop info_ptr;

	// Initialize libpng.
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png_ptr) {
		return -ENOMEM;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, nullptr);
		return -ENOMEM;
	}

	// Initialize the custom I/O handler for MiniZip.
        png_set_write_fn(png_ptr, zfile, d->png_io_minizip_write, d->png_io_minizip_flush);

	// Write to PNG.
	// TODO: If it fails, delete the file from the ZIP?
	int ret = d->writeToPng(png_ptr, info_ptr, img_data, metadata, metaFlags);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	return ret;
}

}
