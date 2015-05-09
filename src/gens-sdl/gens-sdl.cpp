/***************************************************************************
 * gens-sdl: Gens/GS II basic SDL frontend.                                *
 * gens-sdl.cpp: Entry point and main event loop.                          *
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

#include "SdlHandler.hpp"
using GensSdl::SdlHandler;

// LibGens
#include "libgens/lg_main.hpp"
#include "libgens/Rom.hpp"
#include "libgens/MD/EmuMD.hpp"
#include "libgens/Util/MdFb.hpp"
#include "libgens/Util/Timing.hpp"
using LibGens::Rom;
using LibGens::EmuContext;
using LibGens::EmuMD;
using LibGens::MdFb;
using LibGens::Timing;

// LibGensKeys
#include "libgens/IO/IoManager.hpp"
#include "libgens/macros/common.h"
#include "libgenskeys/KeyManager.hpp"
#include "libgenskeys/GensKey_t.h"
using LibGens::IoManager;
using LibGensKeys::KeyManager;

// OS-specific includes.
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include "libgens/Win32/W32U_mini.h"
#else
#include <unistd.h>
#include <pwd.h>
#endif

// yield(), aka usleep(0) or Sleep(0)
#ifdef _WIN32
#define yield() do { Sleep(0); } while (0)
#define usleep(usec) Sleep((DWORD)((usec) / 1000))
#else
#define yield() do { usleep(0); } while (0)
#endif

// C includes.
#include <sys/stat.h>
#include <sys/types.h>

// C includes. (C++ namespace)
#include <cstdio>
#include <cstdlib>

// C++ includes.
#include <string>
using std::string;

#include <SDL.h>

static SdlHandler *sdlHandler = nullptr;
static Rom *rom = nullptr;
static EmuMD *context = nullptr;
static const char *rom_filename = nullptr;

static KeyManager *keyManager = nullptr;
// NOTE: Using SDL keycodes here.
// TODO: Proper SDL to GensKey conversion.
static const GensKey_t keyMap[] = {
	SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT,	// UDLR
	SDLK_s, SDLK_d, SDLK_a, SDLK_RETURN,		// BCAS
	SDLK_e, SDLK_w, SDLK_q, SDLK_RSHIFT		// ZYXM
};

#ifdef _WIN32
#define DIR_SEP_CHR '\\'
#else
#define DIR_SEP_CHR '/'
#endif

/**
 * Recursively create a directory.
 * @param dir Directory.
 * @return 0 on success; non-zero on error.
 */
static int mkdir_recursive(const char *dir) {
	// Reference: http://stackoverflow.com/questions/2336242/recursive-mkdir-system-call-on-unix
	// NOTE: We're not adjusting umask, so mkdir()'s 0777
	// will be reduced to 0755 in most cases.
	char tmp[260];
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", dir);
	len = strlen(tmp);
	if (tmp[len - 1] == DIR_SEP_CHR) {
		tmp[len - 1] = 0;
	}
        for (p = tmp + 1; *p; p++) {
		if (*p == DIR_SEP_CHR) {
			*p = 0;
			// TODO: Check for errors?
			mkdir(tmp, 0777);
			*p = DIR_SEP_CHR;
		}
	}
	mkdir(tmp, 0777);

	// The path should exist now.
	return (access(dir, F_OK));
}

/**
 * Get the configuration directory. (UTF-8 encoding)
 * @return Configuration directory, or nullptr on error.
 */
static const char *getConfigDir(void)
{
	static string config_dir;
	if (config_dir.empty()) {
		// Determine the directory.
#if defined(_WIN32)
		// Windows.
		WCHAR wpath[MAX_PATH];
		if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, wpath))) {
			// Convert from UTF-16 to UTF-8.
			char *upath = W32U_UTF16_to_mbs(wpath, CP_UTF8);
			if (upath) {
				config_dir = string(upath);
				config_dir += "\\gens-gs-ii";
			}
		}
#else
		// TODO: Mac OS X-specific path.
		char *home = getenv("HOME");
		if (home) {
			// HOME variable found. Use it.
			config_dir = string(home);
		} else {
			// HOME variable not found.
			// Check the user's pwent.
			// TODO: Check for getpwuid_r().
			struct passwd *pw = getpwuid(getuid());
			config_dir = string(pw->pw_dir);
		}
		config_dir += "/.config/gens-gs-ii";
#endif
	}

	if (!config_dir.empty()) {
		// Make sure the directory exists.
		if (access(config_dir.c_str(), F_OK) != 0) {
			// Directory does not exist. Create it.
			mkdir_recursive(config_dir.c_str());
		}
	}

	return (!config_dir.empty() ? config_dir.c_str() : nullptr);
}

// Don't use SDL_main.
#undef main
int main(int argc, char *argv[])
{
#ifdef _WIN32
	W32U_Init();
#endif /* _WIN32 */

	if (argc < 2) {
		fprintf(stderr, "usage: %s [rom filename]\n", argv[0]);
		return EXIT_FAILURE;
	}
	rom_filename = argv[1];

	// Make sure we have a valid configuration directory.
	if (getConfigDir() == nullptr) {
		fprintf(stderr, "*** WARNING: Could not find a usable configuration directory.\n"
				"Save functionality will be disabled.\n\n");
	}

	// Initialize SDL.
	int ret = SDL_Init(0);
	if (ret < 0) {
		fprintf(stderr, "SDL initialization failed: %d - %s\n",
			ret, SDL_GetError());
		return EXIT_FAILURE;
	}

#ifdef _WIN32
	// Reference: http://sdl.beuc.net/sdl.wiki/FAQ_Console
	// TODO: Set console as UTF-8.
	freopen("CON", "w", stdout);
	freopen("CON", "w", stderr);
#endif /* _WIN32 */

	// Initialize LibGens.
	LibGens::Init();

	// Load the ROM image.
	rom = new Rom(rom_filename);
	if (!rom->isOpen()) {
		// Error opening the ROM.
		// TODO: Error code?
		fprintf(stderr, "Error opening ROM file %s: (TODO get error code)\n",
			rom_filename);
		return EXIT_FAILURE;
	}
	if (rom->isMultiFile()) {
		// Select the first file.
		rom->select_z_entry(rom->get_z_entry_list());
	}

	// Create the emulation context.
	context = new EmuMD(rom);
	if (!context->isRomOpened()) {
		// Error loading the ROM into EmuMD.
		// TODO: Error code?
		fprintf(stderr, "Error initializing EmuContext for %s: (TODO get error code)\n",
			rom_filename);
		return EXIT_FAILURE;
	}

	// Initialize SDL handlers.
	sdlHandler = new SdlHandler();
	if (sdlHandler->init_video() < 0)
		return EXIT_FAILURE;
	if (sdlHandler->init_audio() < 0)
		return EXIT_FAILURE;

	// Start the frame timer.
	// TODO: Region code?
	LibGens::Timing timing;
	bool isPal = false;
	const unsigned int usec_per_frame = (1000000 / (isPal ? 50 : 60));
	uint64_t start_clk = timing.getTime();
	uint64_t old_clk = start_clk;
	uint64_t fps_clk = start_clk;
	uint64_t new_clk = start_clk;
	// Microsecond counter for frameskip.
	uint64_t usec_frameskip = 0;

	// Frame counters.
	unsigned int frames = 0;
	unsigned int frames_old = 0;
	unsigned int fps = 0;	// TODO: float or double?

	// Enable frameskip.
	bool frameskip = true;

	// TODO: Close the ROM, or let EmuContext do it?

	// Set the color depth.
	MdFb *fb = context->m_vdp->MD_Screen;
	fb->setBpp(MdFb::BPP_32);

	// Set the SDL video source.
	sdlHandler->set_video_source(fb);

	// Set the window title.
	SDL_WM_SetCaption("Gens/GS II [SDL]", nullptr);

	// Start audio.
	SDL_PauseAudio(0);

	// Initialize the I/O Manager with a default key layout.
	keyManager = new KeyManager();
	keyManager->setIoType(IoManager::VIRTPORT_1, IoManager::IOT_6BTN);
	keyManager->setKeyMap(IoManager::VIRTPORT_1, keyMap, ARRAY_SIZE(keyMap));
	keyManager->setIoType(IoManager::VIRTPORT_2, IoManager::IOT_NONE);

	bool running = true;
	bool paused = false;
	while (running) {
		SDL_Event event;
		int ret = (paused
			? SDL_WaitEvent(&event)
			: SDL_PollEvent(&event));
		if (ret) {
			switch (event.type) {
				case SDL_QUIT:
					running = 0;
					break;

				case SDL_KEYDOWN:
					// SDL keycodes nearly match GensKey.
					switch (event.key.keysym.sym) {
						case SDLK_TAB:
							// Check for Shift.
							if (event.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT)) {
								// Hard Reset.
								context->hardReset();
							} else {
								// Soft Reset.
								context->softReset();
							}
							break;
						case SDLK_ESCAPE:
							// Pause emulation.
							// TODO: Apply the pause effect.
							paused = !paused;
							// Reset the clocks and counters.
							frames_old = frames;
							fps = 0;
							start_clk = timing.getTime();
							old_clk = start_clk;
							fps_clk = start_clk;
							new_clk = start_clk;
							// Pause audio.
							SDL_PauseAudio(paused);
							// TODO: Reset the audio ringbuffer?
							// Update the window title.
							if (paused) {
								SDL_WM_SetCaption("Gens/GS II [SDL] [Paused]", nullptr);
							} else {
								SDL_WM_SetCaption("Gens/GS II [SDL]", nullptr);
							}
							break;
						default:
							// Send the key to the KeyManager.
							keyManager->keyDown(event.key.keysym.sym);
							break;
					}

				case SDL_KEYUP:
					// SDL keycodes nearly match GensKey.
					keyManager->keyUp(event.key.keysym.sym);
					break;

				default:
					break;
			}
		}

		if (paused) {
			// Don't do anything.
			continue;
		}

		// New start time.
		new_clk = timing.getTime();

		// Update the FPS counter.
		unsigned int fps_tmp = ((new_clk - fps_clk) & 0x3FFFFF);
		if (fps_tmp >= 1000000) {
			// More than 1 second has passed.
			fps_clk = new_clk;
			if (frames_old > frames) {
				fps = (frames_old - frames);
			} else {
				fps = (frames - frames_old);
			}
			frames_old = frames;

			// Update the window title.
			// TODO: Average the FPS over multiple seconds
			// and/or quarter-seconds.
			char win_title[256];
			snprintf(win_title, sizeof(win_title), "Gens/GS II [SDL] - %d fps", fps);
			SDL_WM_SetCaption(win_title, nullptr);
		}

		// Frameskip.
		if (frameskip) {
			// Determine how many frames to run.
			usec_frameskip += ((new_clk - old_clk) & 0x3FFFFF); // no more than 4 secs
			unsigned int frames_todo = (unsigned int)(usec_frameskip / usec_per_frame);
			usec_frameskip %= usec_per_frame;
			old_clk = new_clk;

			if (frames_todo == 0) {
				// No frames to do yet.
				// Wait until the next frame.
				uint64_t usec_sleep = (usec_per_frame - usec_frameskip);
				if (usec_sleep > 1000) {
					// Never sleep for longer than the 50 Hz value
					// so events are checked often enough.
					if (usec_sleep > (1000000 / 50)) {
						usec_sleep = (1000000 / 50);
					}
					usec_sleep -= 1000;

#ifdef _WIN32
					// Win32: Use a yield() loop.
					// FIXME: Doesn't work properly on VBox/WinXP...
					uint64_t yield_end = timing.getTime() + usec_sleep;
					do {
						yield();
					} while (yield_end > timing.getTime());
#else /* !_WIN32 */
					// Linux: Use usleep().
					usleep(usec_sleep);
#endif /* _WIN32 */
				}
			} else {
				// Draw frames.
				for (; frames_todo != 1; frames_todo--) {
					// Run a frame without rendering.
					context->execFrameFast();
					sdlHandler->update_audio();
				}
				frames_todo = 0;

				// Run a frame and render it.
				context->execFrame();
				sdlHandler->update_audio();
				sdlHandler->update_video();
				// Increment the frame counter.
				frames++;
			}
		} else {
			// Run a frame and render it.
			context->execFrame();
			sdlHandler->update_audio();
			sdlHandler->update_video();
			// Increment the frame counter.
			frames++;
		}

		// Update the I/O manager.
		keyManager->updateIoManager(context->m_ioManager);
	}

	// NOTE: Deleting sdlHandler can cause crashes on Windows
	// due to the timer callback trying to post the semaphore
	// after it's been deleted.
	// Shut down the SDL functions manually.
	sdlHandler->end_audio();
	sdlHandler->end_video();
	//delete sdlHandler;

	// Shut. Down. EVERYTHING.
	delete keyManager;
	delete context;
	delete rom;
	return EXIT_SUCCESS;
}
