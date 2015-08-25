/***************************************************************************
 * libgens: Gens Emulation Library.                                        *
 * byteswap.h: Byteswapping functions.                                     *
 *                                                                         *
 * Copyright (c) 2008-2015 by David Korth                                  *
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

#include "byteswap.h"
#include "cpuflags.h"

// C includes.
#include <assert.h>

/**
 * 16-bit byteswap function.
 * @param Pointer to array to swap.
 * @param n Number of bytes to swap. (Must be divisible by 2; an extra odd byte will be ignored.)
 */
void __byte_swap_16_array(void *ptr, unsigned int n)
{
	unsigned char *cptr = (unsigned char*)ptr;
	unsigned char x;

	// Don't allow uneven lengths.
	assert((n & 1) == 0);
	n &= ~1;

#if defined(__i386__) || defined(__amd64__)
	if (CPU_Flags & MDP_CPUFLAG_X86_MMX) {
		// MMX: Swap 8 bytes (4 words) at a time.
		for (; n > 8; n -= 8, cptr += 8) {
			__asm__ (
				"movq	(%[cptr]), %%mm0\n"
				"movq	%%mm0, %%mm1\n"
				"psllw	$8, %%mm0\n"
				"psrlw	$8, %%mm1\n"
				"por	%%mm0, %%mm1\n"
				"movq	%%mm1, (%[cptr])\n"
				:
				: [cptr] "r" (cptr)
				// FIXME: gcc complains mm? registers are unknown.
				// May need to compile with -mmmx...
				//: "mm0", "mm1"
			);
		}

		// If the block isn't a multiple of 8 bytes,
		// the C implementation will handle the rest.
	} else {
		// Try using 'xchg' instead.
		// Swapping 8 bytes (4 words) at a time.
		for (; n > 8; n -= 8, cptr += 8) {
			__asm__ (
				// NOTE: xorl+movw is slightly faster than
				// movzwl on Core 2 T7200.
				// TODO: Interleaving instructions improved
				// ByteswapTest_benchmark by ~30ms, though
				// it reduces readability.
				// FIXME: xorl *should* improve performance
				// by eliminating partial register stalls,
				// but it actually decreases it slightly...
				#if 0
				"xorl	%%eax, %%eax\n"
				"xorl	%%edx, %%edx\n"
				"xorl	%%esi, %%esi\n"
				"xorl	%%edi, %%edi\n"
				#endif
				"movw	(%[cptr]), %%ax\n"
				"movw	2(%[cptr]), %%dx\n"
				"movw	4(%[cptr]), %%si\n"
				"movw	6(%[cptr]), %%di\n"
				// rol seems to be faster than xchg %al, %ah.
				"rol	$8, %%ax\n"
				"rol	$8, %%dx\n"
				"rol	$8, %%si\n"
				"rol	$8, %%di\n"
				"movw	%%ax, (%[cptr])\n"
				"movw	%%dx, 2(%[cptr])\n"
				"movw	%%si, 4(%[cptr])\n"
				"movw	%%di, 6(%[cptr])\n"
				:
				: [cptr] "r" (cptr)
				: "eax", "edx", "esi", "edi"
			);
		}
	}
#endif

	// C version.
	// Used if MMX isn't available, or if the
	// block isn't a multiple of 8 bytes.
	for (; n > 0; n -= 2, cptr += 2) {
		x = *cptr;
		*cptr = *(cptr + 1);
		*(cptr + 1) = x;
	}
}

/**
 * 32-bit byteswap function.
 * @param Pointer to array to swap.
 * @param n Number of bytes to swap. (Must be divisible by 4; extra bytes will be ignored.)
 */
void __byte_swap_32_array(void *ptr, unsigned int n)
{
	unsigned char *cptr = (unsigned char*)ptr;
	unsigned char x, y;

	// Don't allow lengths that aren't divisible by 4.
	assert((n & 3) == 0);
	n &= ~3;

#if defined(__i386__) || defined(__amd64__)
	// i486+ / amd64: Use 'bswap'.
	// Swap 8 bytes (4 DWORDs) at a time.
	for (; n > 8; n -= 8, cptr += 8) {
		__asm__ (
			"movl	(%[cptr]), %%eax\n"
			"movl	4(%[cptr]), %%edx\n"
			"bswap	%%eax\n"
			"bswap	%%edx\n"
			"movl	%%eax, (%[cptr])\n"
			"movl	%%edx, 4(%[cptr])\n"
			:
			: [cptr] "r" (cptr)
			: "eax", "edx"
		);
	}
	// If the block isn't a multiple of 8 bytes,
	// the C implementation will handle the rest.
#endif

	// C version.
	// Used if optimized asm isn't available, or if the
	// block isn't a multiple of 8 bytes.
	for (; n > 0; n -= 4, cptr += 4) {
		x = *cptr;
		y = *(cptr + 1);
		
		*cptr = *(cptr + 3);
		*(cptr + 1) = *(cptr + 2);
		*(cptr + 2) = y;
		*(cptr + 3) = x;
	}
}
