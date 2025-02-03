// Agrona
// Copyright (c) 2025 CGLJ08. All rights reserved.
// This project includes code derived from Microsoft's MSDN samples. See the LICENSE file for details.

#pragma once

#pragma comment (lib, "d3d11.lib") 	
#pragma comment (lib, "dxgi.lib")
#pragma comment (lib, "dxguid.lib")
#pragma comment (lib, "Dwrite.lib")
#pragma comment (lib, "D2d1.lib")
#pragma comment (lib, "Ole32.lib")
#pragma comment (lib, "xinput.lib")

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>


#include <vector>
#include <memory>
#include <string>
#include <tchar.h>
#include <fstream>
#include <cstddef>
#include <stdio.h>
#include <climits>
#include <stdlib.h>
#include <malloc.h>
#include <iostream>
#include <d2d1_3.h>
#include <Xinput.h>
#include <wbemidl.h>
#include <oleauto.h>
#include <algorithm>
#include <winuser.h>
#include <stdexcept>
#include <dxgi1_4.h> 
#include <d3d11_4.h> 
#include <dwrite_3.h>
#include <wincodec.h> 
#include <ppltasks.h>
#include <combaseapi.h>
#include <wrl\client.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <d3d11sdklayers.h>
#include <Windows.Foundation.h>
#include <wrl\wrappers\corewrappers.h>
