
// Agrona
// Copyright (c) 2025 CGLJ08. All rights reserved.
// This project includes code derived from Microsoft's MSDN samples. See the LICENSE file for details.

#pragma once

// --- Link Libraries ---
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "Dwrite.lib")
#pragma comment (lib, "D2d1.lib")
#pragma comment (lib, "Ole32.lib")
#pragma comment (lib, "xinput.lib")
#pragma comment (lib, "xaudio2.lib") // Added for XAudio2
#pragma comment( lib, "Windowscodecs.lib") // Added for WIC

// --- Windows Includes ---
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winuser.h> // Keep for input messages
#include <Windows.Foundation.h>
#include <wrl\client.h>
#include <wrl\wrappers\corewrappers.h>

// --- Standard Library Includes ---
#include <vector>
#include <memory>
#include <string>
#include <sstream> // Added for string manipulation (e.g., text overlay)
#include <fstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <cstddef>
#include <stdio.h>
#include <climits>
#include <stdlib.h>
#include <malloc.h>
#include <map> // Added for potential asset management
#include <chrono> // Added for timing

// --- DirectX Includes ---
#include <d3d11_4.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <d3d11sdklayers.h> // For debug layer

// --- Direct2D / DirectWrite / WIC Includes ---
#include <d2d1_3.h>
#include <dwrite_3.h>
#include <wincodec.h> // Windows Imaging Component (WIC)

// --- XInput Include ---
#include <Xinput.h> // Gamepad input

// --- XAudio2 Include ---
#include <xaudio2.h> // Audio

// --- Other necessary includes ---
#include <combaseapi.h>
#include <oleauto.h> // Required by WIC? Double check dependency if issues arise.
#include <wbemidl.h> // Keep if needed for system info, otherwise optional
#include <ppltasks.h> // Keep if using async tasks, otherwise optional
#include <tchar.h> // Keep for UNICODE compatibility if needed

// Project specific forward declarations or common types
// (Consider moving AssetTypes.h content here if it's widely used, or include it)

// Define helper macro for releasing COM objects (optional but good practice)
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

// Define max players
constexpr int MAX_PLAYERS = 4;

