// Agrona
// Copyright (c) 2025 CGLJ08. All rights reserved.
// This project includes code derived from Microsoft's MSDN samples. See the LICENSE file for details.

#pragma once

#include "resource.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitializeWindowInstance(HINSTANCE, int, int, int, int, int);

LPCWSTR m_windowTitle;
LPCWSTR m_windowClassName;
HINSTANCE m_hInstance;
HWND m_hWindow;
MSG m_windowMessage;
DEVMODE m_screenDisplaySettings;
float m_screenWidth;
float m_screenHeight;
float m_viewportWidth;
float m_viewportHeight;
UINT m_widthUint = 1600;
UINT m_heightUint = 900;
bool m_exitGame;

