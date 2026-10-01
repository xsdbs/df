#pragma once

#pragma warning( disable : 4307 ) // '*': integral constant overflow
#pragma warning( disable : 4244 ) // possible loss of data
#pragma warning( disable : 4800 ) // forcing value to bool 'true' or 'false'
#pragma warning( disable : 4838 ) // conversion from '::size_t' to 'int' requires a narrowing conversion

// You can define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING or _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS to acknowledge that you have received this warning.
#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

using ulong_t = unsigned long;

// windows / stl includes.
#include <Windows.h>
#include <cstdint>
#include <intrin.h>
#include <xmmintrin.h>
#include <array>
#include <vector>
#include <algorithm>
#include <cctype>
#include <string>
#include <chrono>
#include <thread>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <deque>
#include <functional>
#include <map>
#include <shlobj.h>
#include <filesystem>
#include <streambuf>

// multithread
#include <mutex>

#include <d3d9.h>
#include <d3dx9.h>
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "d3d9.lib")

// our custom wrapper.
#include "unique_vector.h"
#include "tinyformat.h"

// other includes.
#include "hash.h"
#include "xorstr.h"
#include "pe.h"
#include "winapir.h"
#include "address.h"
#include "util.h"
#include "modules.h"
#include "pattern.h"
#include "vmt.h"
#include "nt.h"
#include "x86.h"
#include "syscall.h"

// hack includes.
#include "interfaces.h"
#include "sdk.h"
#include "grimace_thread.h"
#include "csgo.h"
#include "autowall.h"
#include "netvars.h"
#include "entoffsets.h"
#include "entity.h"
#include "server animation.h"
#include "client.h"
#include "gamerules.h"
#include "hooks.h"
#include "render.h"
#include "rebuilt_movement.h"
#include "pred.h"
#include "lagrecord.h"
#include "visuals.h"
#include "movement.h"
#include "bonesetup.h"
#include "hvh.h"
#include "shots.h"
#include "aimbot.h"
#include "netdata.h"
#include "chams.h"
#include "notify.h"
#include "events.h"
#include "resolver.h"
#include "grenades.h"
#include "skins.h"
#include "animations.hpp"

// gui includes.
#include "json.h"
#include "base64.h"
#include "sup_element.h"
#include "sup_checkbox.h"
#include "sup_dropdown.h"
#include "sup_multidropdown.h"
#include "sup_slider.h"
#include "sup_colorpicker.h"
#include "sup_edit.h"
#include "sup_keybind.h"
#include "sup_button.h"
#include "sup_tab.h"
#include "sup_form.h"
#include "sup_gui.h"
#include "sup_callbacks.h"
#include "sup_menu.h"
#include "sup_config.h"