/***************************************************************************
 * libgens: Gens Emulation Library.                                        *
 * Effects.hpp: Special Video Effects. (Software Rendering)                *
 *                                                                         *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville.                      *
 * Copyright (c) 2003-2004 by Stéphane Akhoun.                             *
 * Copyright (c) 2008-2010 by David Korth.                                 *
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
 * NOTE: The video effects here are applied to MD_Screen[].
 */

#ifndef __LIBGENS_UTIL_EFFECTS_HPP__
#define __LIBGENS_UTIL_EFFECTS_HPP__

// RESTRICT keyword.
#if defined(__cplusplus) && defined(__GNUC__)
#define RESTRICT __restrict__
#elif __STDC_VERSION__ >= 199901L
#define RESTRICT restrict
#else
#define RESTRICT
#endif

namespace LibGens
{

class Effects
{
	public:
		enum ColorMask
		{
			CM_BLACK	= 0,
			CM_BLUE		= 1,
			CM_GREEN	= 2,
			CM_CYAN		= 3,
			CM_RED		= 4,
			CM_MAGENTA	= 5,
			CM_YELLOW	= 6,
			CM_WHITE	= 7,
		};
		
		static void DoCrazyEffect(ColorMask colorMask);
		static void DoPausedEffect(void *outScreen);
	
	protected:
		template<typename pixel, pixel Rmask, pixel Gmask, pixel Bmask,
				  pixel Radd, pixel Gadd, pixel Badd>
		static void T_DoCrazyEffect(ColorMask colorMask, pixel *screen);
	
		template<typename pixel, pixel RMask, pixel GMask, pixel BMask,
			 unsigned int RShift, unsigned int GShift, unsigned int BShift>
		static void T_DoPausedEffect(const pixel* RESTRICT mdScreen, pixel* RESTRICT outScreen);
	
	private:
		Effects() { }
		~Effects() { }
};

}

#endif /* __LIBGENS_UTIL_EFFECTS_HPP__ */
