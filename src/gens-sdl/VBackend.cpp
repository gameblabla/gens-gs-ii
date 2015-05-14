/***************************************************************************
 * gens-sdl: Gens/GS II basic SDL frontend.                                *
 * VBackend.hpp: Video Backend base class.                                 *
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

#include "VBackend.hpp"
#include "libgens/Util/MdFb.hpp"

namespace GensSdl {

VBackend::VBackend()
	: m_fb(nullptr)
	, m_stretchMode(STRETCH_H)
{ }

VBackend::~VBackend()
{
	if (m_fb) {
		m_fb->unref();
		m_fb = nullptr;
	}
}

VBackend::StretchMode_t VBackend::stretchMode(void) const
{
	return m_stretchMode;
}

void VBackend::setStretchMode(StretchMode_t stretchMode)
{
	m_stretchMode = stretchMode;
	// TODO: Mark as dirty?
}

}
