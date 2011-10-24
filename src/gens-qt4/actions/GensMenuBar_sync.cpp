/***************************************************************************
 * gens-qt4: Gens Qt4 UI.                                                  *
 * GensMenuBar_sync.cpp: Gens Menu Bar class: Synchronization functions.   *
 *                                                                         *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville.                      *
 * Copyright (c) 2003-2004 by Stéphane Akhoun.                             *
 * Copyright (c) 2008-2011 by David Korth.                                 *
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

#include "GensMenuBar.hpp"
#include "GensMenuBar_p.hpp"
#include "GensMenuBar_menus.hpp"

// gqt4_cfg
#include "gqt4_main.hpp"

// Qt includes.
#include <QtCore/qglobal.h>

// Recent ROMs menu.
#include "RecentRomsMenu.hpp"

namespace GensQt4
{

/*********************************
 * GensMenuBarPrivate functions. *
 *********************************/

/**
 * Connect menu synchronization slots.
 */
void GensMenuBarPrivate::syncConnect(void)
{
	gqt4_cfg->registerChangeNotification(QLatin1String("Graphics/stretchMode"),
					q, SLOT(stretchMode_changed_slot(QVariant)));
	gqt4_cfg->registerChangeNotification(QLatin1String("System/regionCode"),
					q, SLOT(regionCode_changed_slot(QVariant)));
	gqt4_cfg->registerChangeNotification(QLatin1String("Options/enableSRam"),
					q, SLOT(enableSRam_changed_slot(QVariant)));
	gqt4_cfg->registerChangeNotification(QLatin1String("GensWindow/showMenuBar"),
					q, SLOT(showMenuBar_changed_slot(QVariant)));
}


/**
 * Synchronize all menus.
 */
void GensMenuBarPrivate::syncAll(void)
{
	this->lock();
	
	// Do synchronization.
	syncRecent();
	syncShowMenuBar();
	q->stretchMode_changed_slot_int(
			(StretchMode_t)gqt4_cfg->getInt(QLatin1String("Graphics/stretchMode")));
	q->regionCode_changed_slot_int(
			(LibGens::SysVersion::RegionCode_t)gqt4_cfg->getInt(QLatin1String("System/regionCode")));
	q->enableSRam_changed_slot_int(
			gqt4_cfg->get(QLatin1String("Options/enableSRam")).toBool());
	q->stateChanged();
	
	this->unlock();
}


/**
 * Synchronize the "Recent ROMs" menu.
 */
void GensMenuBarPrivate::syncRecent(void)
{
	this->lock();
	
	// Find the "Recent ROMs" action.
	QAction *actionRecentRoms = hashActions.value(IDM_FILE_RECENT);
	if (!actionRecentRoms)
		goto out;
	
	// Set the submenu.
	actionRecentRoms->setMenu(recentRomsMenu);
	
	// If there aren't any ROMs in the list, disable the action.
	actionRecentRoms->setEnabled(!recentRomsMenu->actions().isEmpty());
	
out:
	this->unlock();
}


/**
 * Synchronize the "Show Menu Bar" item.
 */
void GensMenuBarPrivate::syncShowMenuBar(void)
{
#ifndef Q_WS_MAC
	// Show Menu Bar.
	this->lock();
	QAction *actionShowMenuBar = hashActions.value(IDM_GRAPHICS_MENUBAR);
	if (actionShowMenuBar)
		actionShowMenuBar->setChecked(gqt4_cfg->get(QLatin1String("GensWindow/showMenuBar")).toBool());
	this->unlock();
#endif /* Q_WS_MAC */
}


/**************************
 * GensMenuBar functions. *
 **************************/

/** Synchronization slots. **/


/**
 * Recent ROMs menu has been updated.
 */
void GensMenuBar::recentRoms_updated(void)
{
	// Synchronize the "Recent ROMs" menu action.
	d->syncRecent();
}


/**
 * Stretch mode has changed.
 * @param stretchMode New stretch mode.
 */
void GensMenuBar::stretchMode_changed_slot_int(StretchMode_t stretchMode)
{
	if (stretchMode < STRETCH_NONE || stretchMode > STRETCH_FULL)
		return;
	
	// Convert the stretch mode to a menu item ID.
	const int id = (int)stretchMode + IDM_GRAPHICS_STRETCH_NONE;
	
	// Find the action.
	QAction *action = d->hashActions.value(id, NULL);
	if (!action)
		return;
	
	// Set the stretch mode.
	this->lock();
	action->setChecked(true);
	this->unlock();
}

/**
 * Stretch mode has changed.
 * (WRAPPER FUNCTION for stretchMode_changed_slot_int().)
 * @param stretchMode (StretchMode_t) New stretch mode.
 */
void GensMenuBar::stretchMode_changed_slot(const QVariant& stretchMode)
{
	// Wrapper for stretchMode_changed_slot_int().
	stretchMode_changed_slot_int((StretchMode_t)stretchMode.toInt());
}


/**
 * Region code has changed.
 * @param regionCode New region code.
 */
void GensMenuBar::regionCode_changed_slot_int(LibGens::SysVersion::RegionCode_t regionCode)
{
	int id;
	switch (regionCode)
	{
		case LibGens::SysVersion::REGION_AUTO:		id = IDM_SYSTEM_REGION_AUTODETECT; break;
		case LibGens::SysVersion::REGION_JP_NTSC:	id = IDM_SYSTEM_REGION_JAPAN;      break;
		case LibGens::SysVersion::REGION_ASIA_PAL:	id = IDM_SYSTEM_REGION_ASIA;       break;
		case LibGens::SysVersion::REGION_US_NTSC:	id = IDM_SYSTEM_REGION_USA;        break;
		case LibGens::SysVersion::REGION_EU_PAL:	id = IDM_SYSTEM_REGION_EUROPE;     break;
		default:
			break;
	}
	
	// Find the action.
	QAction *action = d->hashActions.value(id, NULL);
	if (!action)
		return;
	
	// Set the region code.
	this->lock();
	action->setChecked(true);
	this->unlock();
}

/**
 * Region code has changed.
 * (WRAPPER FUNCTION for regionCode_changed_slot_int().)
 * @param regionCode (RegionCode_t) New region code.
 */
void GensMenuBar::regionCode_changed_slot(const QVariant& regionCode)
{
	// Wrapper for regionCode_changed_slot_int().
	regionCode_changed_slot_int(
		(LibGens::SysVersion::RegionCode_t)regionCode.toInt());
}


/**
 * Enable SRam/EEPRom setting has changed.
 * @param enableSRam New Enable SRam/EEPRom setting.
 */
void GensMenuBar::enableSRam_changed_slot_int(bool enableSRam)
{
	// Find the action.
	QAction *action = d->hashActions.value(IDM_OPTIONS_ENABLESRAM, NULL);
	if (!action)
		return;
	
	// Set the check state.
	this->lock();
	action->setChecked(enableSRam);
	this->unlock();
}

/**
 * Enable SRam/EEPRom setting has changed.
 * (WRAPPER FUNCTION for enableSRam_changed_slot_int().)
 * @param enableSRam (bool) New Enable SRam/EEPRom setting.
 */
void GensMenuBar::enableSRam_changed_slot(const QVariant& enableSRam)
{
	// Wrapper for enableSRam_changed_slot_int().
	enableSRam_changed_slot_int(enableSRam.toBool());
}


/**
 * "Show Menu Bar" setting has changed.
 * @param newShowMenuBar New "Show Menu Bar" setting.
 */
void GensMenuBar::showMenuBar_changed_slot(const QVariant& newShowMenuBar)
{
	((void)newShowMenuBar);
	d->syncShowMenuBar();
}


/**
 * Emulation state has changed.
 */
void GensMenuBar::stateChanged(void)
{
	// Update various menu items.
	QAction *actionPause = d->hashActions.value(IDM_SYSTEM_PAUSE);
	
	// Lock menu actions to prevent errant signals from being emitted.
	this->lock();
	
	const bool isRomOpen = (d->emuManager && d->emuManager->isRomOpen());
	const bool isPaused = (isRomOpen ? d->emuManager->paused().paused_manual : false);
	
	// TODO: Do we really need to check for NULL actions?
	
	// Update menu actions.
	if (actionPause)
	{
		actionPause->setEnabled(isRomOpen);
		actionPause->setChecked(isPaused);
	}
	
	// Simple enable-if-ROM-open actions.
	static const int EnableIfOpen[] =
	{
		IDM_FILE_CLOSE, IDM_FILE_SAVESTATE, IDM_FILE_LOADSTATE,
		IDM_GRAPHICS_SCRSHOT, IDM_SYSTEM_HARDRESET, IDM_SYSTEM_SOFTRESET,
		0
	};
	
	for (int i = ((sizeof(EnableIfOpen)/sizeof(EnableIfOpen[0]))-2); i >= 0; i--)
	{
		QAction *actionEnableIfOpen = d->hashActions.value(EnableIfOpen[i]);
		if (actionEnableIfOpen)
			actionEnableIfOpen->setEnabled(isRomOpen);
	}
	
	// Z80. (Common for all systems.)
	QAction *actionCpuReset = d->hashActions.value(IDM_SYSTEM_CPURESET_Z80);
	if (actionCpuReset)
	{
		actionCpuReset->setEnabled(isRomOpen);
		actionCpuReset->setVisible(isRomOpen);
	}
	
	// Main 68000. (MD, MCD, 32X, MCD32X)
	// TODO: Change title to "Main 68000" when Sega CD is enabled?
	actionCpuReset = d->hashActions.value(IDM_SYSTEM_CPURESET_M68K);
	if (actionCpuReset)
	{
		actionCpuReset->setEnabled(isRomOpen);
		actionCpuReset->setVisible(isRomOpen);
	}
	
	// Sub 68000. (MCD, MCD32X)
	// TODO: Identify active system.
	actionCpuReset = d->hashActions.value(IDM_SYSTEM_CPURESET_S68K);
	if (actionCpuReset)
	{
		actionCpuReset->setEnabled(false);
		actionCpuReset->setVisible(false);
	}
	
	// Master and Slave SH2. (32X, MCD32X)
	// TODO: Identify active system.
	actionCpuReset = d->hashActions.value(IDM_SYSTEM_CPURESET_MSH2);
	if (actionCpuReset)
	{
		actionCpuReset->setEnabled(false);
		actionCpuReset->setVisible(false);
	}
	actionCpuReset = d->hashActions.value(IDM_SYSTEM_CPURESET_SSH2);
	if (actionCpuReset)
	{
		actionCpuReset->setEnabled(false);
		actionCpuReset->setVisible(false);
	}
	
	// Unlock menu actions.
	this->unlock();
}

}
