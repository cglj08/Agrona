// Agrona
// Copyright (c) 2025 CGLJ08. All rights reserved.
// This project includes code derived from Microsoft's MSDN samples. See the LICENSE file for details.

#include "pch.h"
#include "WinMain.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &m_screenDisplaySettings);

    m_screenWidth = (float)m_screenDisplaySettings.dmPelsWidth;
    m_screenHeight = (float)m_screenDisplaySettings.dmPelsHeight;
    m_viewportWidth = m_screenWidth;
    m_viewportHeight = m_screenHeight;

    m_windowMessage = { };

    auto atomReturned = MyRegisterClass(hInstance);

    if (!atomReturned) { return 1; }

    if (!InitializeWindowInstance(hInstance, nCmdShow, 0, 0, (int)m_widthUint, (int)m_heightUint)) { return 2; }

    while (!m_exitGame)
    {
        while (PeekMessage(&m_windowMessage, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&m_windowMessage);
            DispatchMessage(&m_windowMessage);
        }
    }

    return (int)m_windowMessage.wParam;
}

BOOL InitializeWindowInstance(HINSTANCE hInstance, int nCmdShow, int xPos, int yPos, int windowWidth, int windowHeight)
{

    m_hInstance = hInstance;

    m_hWindow = CreateWindowW(m_windowClassName, m_windowTitle, WS_OVERLAPPEDWINDOW, xPos, yPos, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

    if (!m_hWindow) { return FALSE; }

    if (!IsWindowVisible(m_hWindow))
    {
        ShowWindow(m_hWindow, nCmdShow);
        UpdateWindow(m_hWindow);
    }

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CLOSE:
    {
        if (MessageBox(m_hWindow, L"Would you like to exit Agrona?", L"Agrona", MB_OKCANCEL) == IDOK) { DestroyWindow(m_hWindow); }
        return 0;
    }
    case WM_SIZE:
    {
        if (wParam == SIZE_MINIMIZED) { return 0; }

    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
    }
    break;
    case WM_DESTROY:
    {
        m_exitGame = true;
        PostQuitMessage(911);
    }
    break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{

    WNDCLASSEXW wcex{};

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = m_windowClassName;
    wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2));

    return RegisterClassExW(&wcex);
}