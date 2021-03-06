/***************************************************************************
 * gens-qt4: Gens Qt4 UI.                                                  *
 * KeyHandlerQt.cpp: Qt key remapping handler.                             *
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

#include "KeyHandlerQt.hpp"

// Gens Menu shortcuts.
#include "gqt4_main.hpp"
#include "windows/GensMenuShortcuts.hpp"

// Key Manager.
#include "libgenskeys/KeyManager.hpp"

// Qt includes
#include <QtCore/qglobal.h>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QAction>

// C includes. (C++ namespace)
#include <cstring>

// Native virtual keycodes.
#if defined(Q_WS_X11)
#include <X11/keysym.h>
#elif defined(Q_WS_WIN)
#include <windows.h>
#endif

namespace GensQt4 {

/**
 * Initialize KeyHandlerQt.
 * @param keyManager Key Manager.
 * NOTE: This class does NOT delete keyManager on shutdown!
 */
KeyHandlerQt::KeyHandlerQt(QObject *parent, LibGensKeys::KeyManager *keyManager)
	: super(parent)
	, m_keyManager(keyManager)
{ }

/**
 * Shut down KeyHandlerQt.
 */
KeyHandlerQt::~KeyHandlerQt(void)
{ }

/**
 * Key press handler.
 * @param event Key event.
 */
void KeyHandlerQt::keyPressEvent(QKeyEvent *event)
{
	// TODO: Move effects keypresses from GensQGLWidget to KeyHandlerQt.
	// TODO: Multiple keyboard support?
	if (event->isAutoRepeat())
		return;

	GensKey_t gensKey = QKeyEventToKeyVal(event);
	if (gensKey == KEYV_UNKNOWN)
		return;

	// TODO: Split GensMenuShortcuts into config / actions.
	// If this is an event key, don't handle it as a controller key.
	// We need to apply the modifiers for this to work.
	// Qt's modifiers conveniently map to GensKeyMod_t.
	// TODO: Use GensKeyM_t to indicate modifiers?
	GensKey_t gensKeyMod = (gensKey | ((event->modifiers() >> 16) & 0x1E00));
	QAction *action = gqt4_cfg->gensMenuShortcuts()->keyToAction(gensKeyMod);
	if (action != nullptr) {
		// Key is an event key.
		action->trigger();
		return;
	}

	// Not an event key. Mark it as pressed.
	if (m_keyManager) {
		m_keyManager->keyDown(gensKey);
	}
}

/**
 * Key release handler.
 * @param event Key event.
 */
void KeyHandlerQt::keyReleaseEvent(QKeyEvent *event)
{
	// TODO: Multiple keyboard support?
	if (event->isAutoRepeat())
		return;

	int gensKey = QKeyEventToKeyVal(event);
	if (m_keyManager) {
		m_keyManager->keyUp(gensKey);
	}
}

/**
 * Mouse movement handler.
 * TODO: This function is broken!
 * @param event Mouse event.
 */
void KeyHandlerQt::mouseMoveEvent(QMouseEvent *event)
{
	// TODO
#if 0
	if (!gqt4_emuThread) {
		m_lastMousePosValid = false;
		return;
	}

	if (!m_lastMousePosValid) {
		// Last mouse movement event was invalid.
		m_lastMousePos = event->pos();
		m_lastMousePosValid = true;
		return;
	}

	// Calculate the relative movement.
	QPoint posDiff = (event->pos() - m_lastMousePos);
	m_lastMousePos = event->pos();

	// Forward the relative movement to the I/O devices.
	// NOTE: Port E isn't forwarded, since it isn't really usable as a controller.
	LibGens::EmuMD::m_port1->mouseMove(posDiff.x(), posDiff.y());
	LibGens::EmuMD::m_port2->mouseMove(posDiff.x(), posDiff.y());
#endif
}

/**
 * Mouse button press handler.
 * @param event Mouse event.
 */
void KeyHandlerQt::mousePressEvent(QMouseEvent *event)
{
	int gensButton;
	switch (event->button()) {
		case Qt::NoButton:	return;
		case Qt::LeftButton:	gensButton = MBTN_LEFT; break;
		case Qt::MidButton:	gensButton = MBTN_MIDDLE; break;
		case Qt::RightButton:	gensButton = MBTN_RIGHT; break;
		case Qt::XButton1:	gensButton = MBTN_X1; break;
		case Qt::XButton2:	gensButton = MBTN_X2; break;
		default:		gensButton = MBTN_UNKNOWN; break;
	}

	// Mark the key as pressed.
	if (m_keyManager) {
		m_keyManager->keyDown(KEYV_MOUSE_UNKNOWN + gensButton);
	}
}


/**
 * Mouse button release handler.
 * @param event Mouse event.
 */
void KeyHandlerQt::mouseReleaseEvent(QMouseEvent *event)
{
	int gensButton;
	switch (event->button())
	{
		case Qt::NoButton:	return;
		case Qt::LeftButton:	gensButton = MBTN_LEFT; break;
		case Qt::MidButton:	gensButton = MBTN_MIDDLE; break;
		case Qt::RightButton:	gensButton = MBTN_RIGHT; break;
		case Qt::XButton1:	gensButton = MBTN_X1; break;
		case Qt::XButton2:	gensButton = MBTN_X2; break;
		default:		gensButton = MBTN_UNKNOWN; break;
	}

	// Mark the key as pressed.
	if (m_keyManager) {
		m_keyManager->keyUp(KEYV_MOUSE_UNKNOWN + gensButton);
	}
}

/**
 * Convert a QKeyEvent to a LibGens key value.
 * TODO: Move somewhere else?
 * @param event QKeyEvent.
 * @return LibGens key value. (0 for unknown; -1 for unhandled left/right modifier key.)
 */
GensKey_t KeyHandlerQt::QKeyEventToKeyVal(QKeyEvent *event)
{
	using namespace LibGens;
	
	// Table of Qt::Keys in range 0x00-0x7F.
	// (Based on Qt 4.6.3)
	static const int QtKey_Ascii[0x80] = {
		// 0x00
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		
		// 0x20
		KEYV_SPACE, KEYV_EXCLAIM, KEYV_QUOTEDBL, KEYV_HASH,
		KEYV_DOLLAR, KEYV_PERCENT, KEYV_AMPERSAND, KEYV_QUOTE,
		KEYV_LEFTPAREN, KEYV_RIGHTPAREN, KEYV_ASTERISK, KEYV_PLUS,
		KEYV_COMMA, KEYV_MINUS, KEYV_PERIOD, KEYV_SLASH,
		
		// 0x30
		KEYV_0, KEYV_1, KEYV_2, KEYV_3, KEYV_4, KEYV_5, KEYV_6, KEYV_7,
		KEYV_8, KEYV_9, KEYV_COLON, KEYV_SEMICOLON,
		KEYV_LESS, KEYV_EQUALS, KEYV_GREATER, KEYV_QUESTION,
		
		// 0x40
		KEYV_AT, KEYV_a, KEYV_b, KEYV_c, KEYV_d, KEYV_e, KEYV_f, KEYV_g,
		KEYV_h, KEYV_i, KEYV_j, KEYV_k, KEYV_l, KEYV_m, KEYV_n, KEYV_o,
		KEYV_p, KEYV_q, KEYV_r, KEYV_s, KEYV_t, KEYV_u, KEYV_v, KEYV_w,
		KEYV_x, KEYV_y, KEYV_z, KEYV_LEFTBRACKET,
		KEYV_BACKSLASH, KEYV_RIGHTBRACKET, KEYV_CARET, KEYV_UNDERSCORE,
		
		// 0x60
		KEYV_BACKQUOTE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEYV_BRACELEFT,
		KEYV_BAR, KEYV_BRACERIGHT, KEYV_TILDE, KEYV_DELETE,
	};
	
	// Table of Qt::Keys in range 0x01000000-0x0100007F.
	// (Based on Qt 4.6.3)
	// TODO: Check how numpad keys act with numlock on/off!
	// NOTE: Keys with a value of -1 aren't handled by Qt. (left/right modifiers)
	// NOTE: Media keys are not included.
	static const int QtKey_Extended[0x80] = {
		// 0x01000000
		KEYV_ESCAPE, KEYV_TAB, KEYV_TAB, KEYV_BACKSPACE,
		KEYV_RETURN, KEYV_KP_ENTER, KEYV_INSERT, KEYV_DELETE,
		KEYV_PAUSE, KEYV_PRINT, KEYV_SYSREQ, KEYV_CLEAR,
		0, 0, 0, 0,
		
		// 0x01000010
		KEYV_HOME, KEYV_END, KEYV_LEFT, KEYV_UP,
		KEYV_RIGHT, KEYV_DOWN, KEYV_PAGEUP, KEYV_PAGEDOWN,
		0, 0, 0, 0, 0, 0, 0, 0,
		
		// 0x01000020
		-1, -1, -1, -1, KEYV_CAPSLOCK, KEYV_NUMLOCK, KEYV_SCROLLLOCK, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		
		// 0x01000030
		KEYV_F1, KEYV_F2, KEYV_F3, KEYV_F4, KEYV_F5, KEYV_F6, KEYV_F7, KEYV_F8,
		KEYV_F9, KEYV_F10, KEYV_F11, KEYV_F12, KEYV_F13, KEYV_F14, KEYV_F15, KEYV_F16,
		KEYV_F17, KEYV_F18, KEYV_F19, KEYV_F20, KEYV_F21, KEYV_F22, KEYV_F23, KEYV_F24,
		KEYV_F25, KEYV_F26, KEYV_F27, KEYV_F28, KEYV_F29, KEYV_F30, KEYV_F31, KEYV_F32,
		
		// 0x01000050
		0, 0, 0, -1, -1, KEYV_MENU, -1, -1,
		KEYV_HELP, 0, 0, 0, 0, 0, 0, 0,
		
		// 0x01000060
		0, KEYV_BACK, KEYV_FORWARD, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0
	};
	
	if (event->modifiers() & Qt::KeypadModifier) {
		// Numeric keypad key.
		// TODO: Optimize this to use lookup tables instead of switch/case.
		switch (event->key()) {
#ifdef Q_WS_MAC
			case Qt::Key_0: case Qt::Key_Insert:	return KEYV_KP0;
			case Qt::Key_1: case Qt::Key_End:	return KEYV_KP1;
			case Qt::Key_2:				return KEYV_KP2;
			case Qt::Key_3: case Qt::Key_PageDown:	return KEYV_KP3;
			case Qt::Key_4: 			return KEYV_KP4;
			// TODO: I don't think Qt::Key_Clear is needed here...
			case Qt::Key_5: case Qt::Key_Clear:	return KEYV_KP5;
			case Qt::Key_6: 			return KEYV_KP6;
			case Qt::Key_7: case Qt::Key_Home:	return KEYV_KP7;
			case Qt::Key_8:				return KEYV_KP8;
			case Qt::Key_9: case Qt::Key_PageUp:	return KEYV_KP9;
			
			// Mac OS X: Arrow keys map to the numeric keypad.
			case Qt::Key_Down:			return KEYV_DOWN;
			case Qt::Key_Left:			return KEYV_LEFT;
			case Qt::Key_Right:			return KEYV_RIGHT;
			case Qt::Key_Up:			return KEYV_UP;
#else /* !Q_WS_MAC */
			// Win32, Linux: Arrows map normally.
			case Qt::Key_0: case Qt::Key_Insert:	return KEYV_KP0;
			case Qt::Key_1: case Qt::Key_End:	return KEYV_KP1;
			case Qt::Key_2: case Qt::Key_Down:	return KEYV_KP2;
			case Qt::Key_3: case Qt::Key_PageDown:	return KEYV_KP3;
			case Qt::Key_4: case Qt::Key_Left:	return KEYV_KP4;
			case Qt::Key_5: case Qt::Key_Clear:	return KEYV_KP5;
			case Qt::Key_6: case Qt::Key_Right:	return KEYV_KP6;
			case Qt::Key_7: case Qt::Key_Home:	return KEYV_KP7;
			case Qt::Key_8: case Qt::Key_Up:	return KEYV_KP8;
			case Qt::Key_9: case Qt::Key_PageUp:	return KEYV_KP9;
#endif /* Q_WS_MAC */
			case Qt::Key_Period: case Qt::Key_Delete: return KEYV_KP_PERIOD;
			case Qt::Key_Slash:			return KEYV_KP_DIVIDE;
			case Qt::Key_Asterisk:			return KEYV_KP_MULTIPLY;
			case Qt::Key_Minus:			return KEYV_KP_MINUS;
			case Qt::Key_Plus:			return KEYV_KP_PLUS;
			case Qt::Key_Enter:			return KEYV_KP_ENTER;
			// TODO: Verify on Mac OS X.
			case Qt::Key_Equal:			return KEYV_KP_EQUALS;
			default:	break;
		}
	}
	
	int key = event->key();
	switch (key & ~0x7F) {
		case 0x00000000:
			// ASCII key.
			// TODO: Make sure Qt doesn't apply shifting!
			return QtKey_Ascii[key];
		
		case 0x01000000: {
			// Extended key.
			int gensKey = QtKey_Extended[key & 0x7F];
			if (gensKey >= 0)
				return gensKey;
			
			// Extended key is not handled by Qt.
			// This happens with e.g. left/right shift.
			// (Qt reports both keys as the same.)
			return NativeModifierToKeyVal(event);
		}

		default:
			// Other key.
			switch (key) {
				case Qt::Key_AltGr:
					return KEYV_MODE;
				case Qt::Key_Multi_key:
					return KEYV_COMPOSE;
				default:
					return 0;
			}
	}
}

/**
 * Convert a native virtual key for a modifier to a LibGens key value.
 * TODO: Move somewhere else?
 * @param event QKeyEvent.
 * @return LibGens key value. (0 for unknown)
 */
GensKey_t KeyHandlerQt::NativeModifierToKeyVal(QKeyEvent *event)
{
	using namespace LibGens;
	
#if defined(Q_WS_X11)
	// X11 keysym.
	switch (event->nativeVirtualKey()) {
		case XK_Shift_L:	return KEYV_LSHIFT;
		case XK_Shift_R:	return KEYV_RSHIFT;
		case XK_Control_L:	return KEYV_LCTRL;
		case XK_Control_R:	return KEYV_RCTRL;
		case XK_Meta_L:		return KEYV_LMETA;
		case XK_Meta_R:		return KEYV_RMETA;
		case XK_Alt_L:		return KEYV_LALT;
		case XK_Alt_R:		return KEYV_RALT;
		case XK_Super_L:	return KEYV_LSUPER;
		case XK_Super_R:	return KEYV_RSUPER;
		case XK_Hyper_L:	return KEYV_LHYPER;
		case XK_Hyper_R:	return KEYV_RHYPER;
		default:		break;
	}
#elif defined(Q_WS_WIN)
	// Win32 virtual key.
	// NOTE: Shift, Control, and Alt are NOT tested here.
	// WM_KEYDOWN/WM_KEYUP report VK_SHIFT, VK_CONTORL, and VK_MENU (Alt).
	// These are useless for testing left/right keys.
	// Instead, GetAsyncKeyState() is used in LibGensKeys::KeyManager::updateIoManager().
	switch (event->nativeVirtualKey()) {
		case VK_LWIN:		return KEYV_LSUPER;
		case VK_RWIN:		return KEYV_RSUPER;
		default:		break;
	}
#elif defined(Q_WS_MAC)
	/**
	 * FIXME: Mac OS X doesn't allow user applications to
	 * determine if left or right modifier keys are pressed.
	 * (There should be flag somewhere that enables it,
	 * since there are constants defined for both left
	 * and right keys, but I can't seem to find them...)
	 * 
	 * For now, just use Left keys. (default handler)
	 */
#else
	// Unhandled system.
	#warning Unhandled system; modifier keys will fall back to Left variants!!
#endif
	
	// Unhandled key. Return left key by default.
	switch (event->key()) {
		case Qt::Key_Shift:	return KEYV_LSHIFT;
#ifdef Q_OS_MAC
		// Qt/Mac remaps some keys:
		// Qt::Key_Control == Command
		// Qt::Key_Meta == Control
		case Qt::Key_Control:	return KEYV_LSUPER;
		case Qt::Key_Meta:	return KEYV_LCTRL;
#else
		case Qt::Key_Control:	return KEYV_LCTRL;
		case Qt::Key_Meta:	return KEYV_LMETA;
#endif /* Q_OS_MAC */
		case Qt::Key_Alt:	return KEYV_LALT;
		case Qt::Key_Super_L:	return KEYV_LSUPER;
		case Qt::Key_Super_R:	return KEYV_RSUPER;
		case Qt::Key_Hyper_L:	return KEYV_LHYPER;
		case Qt::Key_Hyper_R:	return KEYV_RHYPER;
		default:		return KEYV_UNKNOWN;
	}
}

/**
 * Convert a GensKey_t to a Qt key value, with GensKey modifiers.
 * @param keyM Gens keycode, with modifiers.
 * @return Qt key value, or 0 on error.
 */
int KeyHandlerQt::KeyValMToQtKey(GensKey_t keyM)
{
	GensKey_u keyU;
	keyU.keycode = keyM;
	if (keyU.type != 0)
		return 0;

	// Get the modifiers first.
	// TODO: Convert GensKeys with numeric keypad modifier.
	// (There are dedicated key values for numpad keys,
	// but Qt uses a modifier so it may get confused.)
	int qtKey = (keyM & 0x1E00) << 16;

	// Determine the key.
	keyM &= 0x1FF;
	if (keyM > KEYV_LAST)
		return 0;
	else if (keyM == KEYV_FORWARD)
		return (qtKey | Qt::Key_Forward);
	else if (keyM == KEYV_BACK)
		return (qtKey | Qt::Key_Back);

	// Other keys. Use a lookup table.
	// TODO: Numeric keypad modifier.
	static const int keyvalMtoQtKey_tbl[0x100] = {
		// 0x00
		0, 0, 0, 0, 0, 0, 0, 0,
		Qt::Key_Backspace, Qt::Key_Tab, 0, 0,
		Qt::Key_Clear, Qt::Key_Return, 0, 0,
		0, 0, 0, Qt::Key_Pause, 0, 0, 0, 0,
		0, 0, 0, Qt::Key_Escape, 0, 0, 0, 0,

		// 0x20
		Qt::Key_Space, Qt::Key_Exclam, Qt::Key_QuoteDbl, Qt::Key_NumberSign,
		Qt::Key_Dollar, Qt::Key_Percent, Qt::Key_Ampersand, Qt::Key_Apostrophe,
		Qt::Key_ParenLeft, Qt::Key_ParenRight, Qt::Key_Asterisk, Qt::Key_Plus,
		Qt::Key_Comma, Qt::Key_Minus, Qt::Key_Period, Qt::Key_Slash,

		// 0x30
		Qt::Key_0, Qt::Key_1, Qt::Key_2, Qt::Key_3,
		Qt::Key_4, Qt::Key_5, Qt::Key_6, Qt::Key_7,
		Qt::Key_8, Qt::Key_9, Qt::Key_Colon, Qt::Key_Semicolon,
		Qt::Key_Less, Qt::Key_Equal, Qt::Key_Greater, Qt::Key_Question,

		// 0x40
		Qt::Key_At, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, Qt::Key_BracketLeft,
		Qt::Key_Backslash, Qt::Key_BracketRight, Qt::Key_AsciiCircum, Qt::Key_Underscore,

		// 0x60
		Qt::Key_QuoteLeft, Qt::Key_A, Qt::Key_B, Qt::Key_C, Qt::Key_D, Qt::Key_E, Qt::Key_F, Qt::Key_G,
		Qt::Key_H, Qt::Key_I, Qt::Key_J, Qt::Key_K, Qt::Key_L, Qt::Key_M, Qt::Key_N, Qt::Key_O,
		Qt::Key_P, Qt::Key_Q, Qt::Key_R, Qt::Key_S, Qt::Key_T, Qt::Key_U, Qt::Key_V, Qt::Key_W,
		Qt::Key_X, Qt::Key_Y, Qt::Key_Z, Qt::Key_BraceLeft,
		Qt::Key_Bar, Qt::Key_BraceRight, Qt::Key_AsciiTilde, Qt::Key_Delete,

		// 0x80: Numeric keypad.
		// TODO: Verify numeric keypad modifier.
		(Qt::KeypadModifier | Qt::Key_0), (Qt::KeypadModifier | Qt::Key_1),
		(Qt::KeypadModifier | Qt::Key_2), (Qt::KeypadModifier | Qt::Key_3),
		(Qt::KeypadModifier | Qt::Key_4), (Qt::KeypadModifier | Qt::Key_5),
		(Qt::KeypadModifier | Qt::Key_6), (Qt::KeypadModifier | Qt::Key_7),
		(Qt::KeypadModifier | Qt::Key_8), (Qt::KeypadModifier | Qt::Key_9),
		(Qt::KeypadModifier | Qt::Key_Period), (Qt::KeypadModifier | Qt::Key_Slash),
		(Qt::KeypadModifier | Qt::Key_Asterisk), (Qt::KeypadModifier | Qt::Key_Minus),
		(Qt::KeypadModifier | Qt::Key_Plus), (Qt::KeypadModifier | Qt::Key_Enter),

		// 0x90
		(Qt::KeypadModifier | Qt::Key_Equal), Qt::Key_Up,
		Qt::Key_Down, Qt::Key_Right,
		Qt::Key_Left, Qt::Key_Insert, Qt::Key_Home, Qt::Key_End,
		Qt::Key_PageUp, Qt::Key_PageDown, 0, 0,
		0, 0, 0, 0,

		// 0xA0: Function keys. (F1-F16)
		Qt::Key_F1, Qt::Key_F2, Qt::Key_F3, Qt::Key_F4,
		Qt::Key_F5, Qt::Key_F6, Qt::Key_F7, Qt::Key_F8,
		Qt::Key_F9, Qt::Key_F10, Qt::Key_F11, Qt::Key_F12,
		Qt::Key_F13, Qt::Key_F14, Qt::Key_F15, Qt::Key_F16,

		// 0xB0: Function keys. (F17-F32)
		Qt::Key_F17, Qt::Key_F18, Qt::Key_F19, Qt::Key_F20,
		Qt::Key_F21, Qt::Key_F22, Qt::Key_F23, Qt::Key_F24,
		Qt::Key_F25, Qt::Key_F26, Qt::Key_F27, Qt::Key_F28,
		Qt::Key_F29, Qt::Key_F30, Qt::Key_F31, Qt::Key_F32,

		// 0xC0: Key state modifier keys.
		// TODO: Left/Right modifiers.
		Qt::Key_NumLock, Qt::Key_CapsLock, Qt::Key_ScrollLock, Qt::Key_Shift,
		Qt::Key_Shift, Qt::Key_Control, Qt::Key_Control, Qt::Key_Alt,
		Qt::Key_Alt, Qt::Key_Meta, Qt::Key_Meta, Qt::Key_Super_L,
		Qt::Key_Super_R, Qt::Key_AltGr, Qt::Key_Multi_key, Qt::Key_Hyper_L,

		// 0xD0
		Qt::Key_Hyper_R, Qt::Key_Direction_L, Qt::Key_Direction_R, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

		// 0xE0: Miscellaneous function keys.
		// TODO: KEYV_BREAK, Key_PowerOff vs. Key_PowerDown; KEYV_EURO, KEYV_UNDO
		Qt::Key_Help, Qt::Key_Print, Qt::Key_SysReq, 0,
		Qt::Key_Menu, Qt::Key_PowerOff, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,

		// 0xF0: Mouse buttons.
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0
	};

	qtKey |= keyvalMtoQtKey_tbl[keyM & 0xFF];
	return qtKey;
}

}
