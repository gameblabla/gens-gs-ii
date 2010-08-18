/***************************************************************************
 * gens-qt4: Gens Qt4 UI.                                                  *
 * ABackend_Write.hpp: Audio Write functions.                              *
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

#include "GensPortAudio.hpp"

// C includes.
#include <string.h>
#include <unistd.h>

// LOG_MSG() subsystem.
#include "libgens/macros/log_msg.h"

// LibGens Sound Manager.
#include "libgens/sound/SoundMgr.hpp"

namespace GensQt4
{

/**
 * write(): Write the current segment to the audio buffer.
 */
int GensPortAudio::write(void)
{
	if (!m_open)
		return 1;
	
	const int SegLength = LibGens::SoundMgr::GetSegLength();
	while ((m_bufPos + SegLength) > m_bufLen)
	{
		// Buffer overflow! Wait for the buffer to decrease.
		// This seems to help limit the framerate when it's running too fast.
		// TODO: Move somewhere else?
		// TODO: usleep() or not?
		// TODO: Make it configurable?
		usleep(1000);
	}
	
	// Lock the audio buffer.
	m_mtxBuf.lock();
	
	// TODO: Mono/stereo, MMX, etc.
	unsigned int i = 0;
	unsigned int bufIndex = m_bufPos;
	for (; i < SegLength && bufIndex < m_bufLen; i++, bufIndex++)
	{
		int32_t L = LibGens::SoundMgr::ms_SegBufL[i];
		int32_t R = LibGens::SoundMgr::ms_SegBufR[i];
		
		if (L < -0x8000)
			m_buf[bufIndex][0] = -0x8000;
		else if (L > 0x7FFF)
			m_buf[bufIndex][0] = 0x7FFF;
		else
			m_buf[bufIndex][0] = (int16_t)L;
		
		if (R < -0x8000)
			m_buf[bufIndex][1] = -0x8000;
		else if (R > 0x7FFF)
			m_buf[bufIndex][1] = 0x7FFF;
		else
			m_buf[bufIndex][1] = (int16_t)R;
		
		// Remove the sample from the segment buffers.
		// TODO: Maybe use memset() after everything's done.
		LibGens::SoundMgr::ms_SegBufL[i] = 0;
		LibGens::SoundMgr::ms_SegBufR[i] = 0;
	}
	
	// Increase the buffer position.
	m_bufPos += i;
	
	// Unlock the audio buffer.
	m_mtxBuf.unlock();
	return 0;
}

}
