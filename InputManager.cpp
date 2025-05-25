// Agrona
// Copyright (c) 2025 CGLJ08. All rights reserved.
// This project includes code derived from Microsoft's MSDN samples. See the LICENSE file for details.

#include "pch.h"
#include "InputManager.h"
#include <limits> // For numeric_limits

InputManager::InputManager() {
    ZeroMemory(m_keyboardState, sizeof(m_keyboardState));
    ZeroMemory(m_prevKeyboardState, sizeof(m_prevKeyboardState));
    ZeroMemory(m_mouseButtonState, sizeof(m_mouseButtonState));
    ZeroMemory(m_prevMouseButtonState, sizeof(m_prevMouseButtonState));
    ZeroMemory(m_gamepadState, sizeof(m_gamepadState));
    ZeroMemory(m_prevGamepadState, sizeof(m_prevGamepadState));
    ZeroMemory(m_gamepadConnected, sizeof(m_gamepadConnected));
}

InputManager::~InputManager() {
    Shutdown();
}

bool InputManager::Initialize(HWND hwnd) {
    m_hwnd = hwnd;
    if (!m_hwnd) return false;

    // Get initial keyboard state
    if (!GetKeyboardState(m_keyboardState)) {
        return false;
    }
    memcpy(m_prevKeyboardState, m_keyboardState, sizeof(m_keyboardState));

    // Get initial mouse state (position is relative to screen initially)
    UpdateMousePosition();
    m_prevMousePos = m_mousePos;

    // Enable XInput (optional, can be called later if needed)
    XInputEnable(TRUE);

    // Setup Raw Input for Mouse (Requires modification in WndProc)
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01; // Generic Desktop Controls
    rid.usUsage = 0x02;     // Mouse
    rid.dwFlags = RIDEV_INPUTSINK; // Receive input even when not foreground (or use 0 for foreground only)
    rid.hwndTarget = m_hwnd;
    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
       // Handle error - raw input might not be available or fail
       OutputDebugString(L"Failed to register raw input device.\n");
       // Proceeding without raw input is possible, but less ideal for FPS camera
    }

    return true;
}

void InputManager::Shutdown() {
    // Disable XInput
    XInputEnable(FALSE);
    m_hwnd = nullptr; // Don't destroy the HWND here, it's owned elsewhere

    // Unregister Raw Input (Optional - depends on lifetime management)
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = RIDEV_REMOVE; // Flag to remove registration
    rid.hwndTarget = nullptr; // Must be NULL on removal
    RegisterRawInputDevices(&rid, 1, sizeof(rid)); // Ignore return value on removal
}

void InputManager::UpdateMousePosition() {
     if (m_captureMouse) {
         // Keep mouse centered logic (or handle delta from raw input)
         RECT windowRect;
         GetWindowRect(m_hwnd, &windowRect);
         int centerX = windowRect.left + (windowRect.right - windowRect.left) / 2;
         int centerY = windowRect.top + (windowRect.bottom - windowRect.top) / 2;
         SetCursorPos(centerX, centerY);
         // Store the center position so delta calculation is relative to it
         m_mousePos.x = centerX;
         m_mousePos.y = centerY;
     } else {
        // Get cursor position relative to the screen
        GetCursorPos(&m_mousePos);
        // Convert to client coordinates if needed (depends on how you use it)
         ScreenToClient(m_hwnd, &m_mousePos);
     }
}

void InputManager::Update() {
    // --- Keyboard ---
    // Store previous state
    memcpy(m_prevKeyboardState, m_keyboardState, sizeof(m_keyboardState));
    // Get current state
    if (!GetKeyboardState(m_keyboardState)) {
        // Handle error?
        OutputDebugString(L"GetKeyboardState failed!\n");
    }

    // --- Mouse ---
    m_prevMousePos = m_mousePos; // Store position from end of last frame or last update
    m_mouseDelta = { 0, 0 };     // Reset delta (will be accumulated by Raw Input or calculated below)
    m_mouseWheelDelta = 0; // Reset wheel delta (accumulated in WndProc)
    m_mouseMovedSinceUpdate = false; // Reset flag

    // Store previous button state
    memcpy(m_prevMouseButtonState, m_mouseButtonState, sizeof(m_mouseButtonState));
    // Get current button state
    m_mouseButtonState[0] = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
    m_mouseButtonState[1] = (GetKeyState(VK_RBUTTON) & 0x8000) != 0;
    m_mouseButtonState[2] = (GetKeyState(VK_MBUTTON) & 0x8000) != 0;

    // Update mouse position *after* potentially getting raw input delta
    // This ensures m_prevMousePos is correct if not using raw input
    if (!m_captureMouse) { // Only update position if not capturing (raw input handles delta)
        POINT currentScreenPos;
        GetCursorPos(&currentScreenPos);
        ScreenToClient(m_hwnd, &currentScreenPos);
        m_mousePos = currentScreenPos; // Update current client pos

        // Calculate delta if not using raw input
        m_mouseDelta.x = m_mousePos.x - m_prevMousePos.x;
        m_mouseDelta.y = m_mousePos.y - m_prevMousePos.y;
    }
     else {
         // When capturing, m_mouseDelta is accumulated in ProcessRawMouseInput
         // Reset position for next frame's centering
          UpdateMousePosition(); // Recenter
          m_prevMousePos = m_mousePos; // Set previous to the center
     }


    // --- Gamepad ---
    for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i) {
        // Store previous state
        m_prevGamepadState[i] = m_gamepadState[i];

        // Get current state
        ZeroMemory(&m_gamepadState[i], sizeof(XINPUT_STATE));
        DWORD result = XInputGetState(i, &m_gamepadState[i]);

        m_gamepadConnected[i] = (result == ERROR_SUCCESS);
    }
}

// --- Keyboard Methods ---
bool InputManager::IsKeyDown(int vkCode) const {
    return (m_keyboardState[vkCode] & 0x80) != 0;
}

bool InputManager::IsKeyJustPressed(int vkCode) const {
    return ((m_keyboardState[vkCode] & 0x80) != 0) && !((m_prevKeyboardState[vkCode] & 0x80) != 0);
}

bool InputManager::IsKeyJustReleased(int vkCode) const {
     return !((m_keyboardState[vkCode] & 0x80) != 0) && ((m_prevKeyboardState[vkCode] & 0x80) != 0);
}


// --- Mouse Methods ---
POINT InputManager::GetMousePosition() const {
    return m_mousePos;
}

POINT InputManager::GetMouseDelta() const {
    return m_mouseDelta;
}

bool InputManager::IsMouseButtonDown(int button) const {
    if (button < 0 || button > 2) return false;
    return m_mouseButtonState[button];
}

bool InputManager::IsMouseButtonJustPressed(int button) const {
    if (button < 0 || button > 2) return false;
    return m_mouseButtonState[button] && !m_prevMouseButtonState[button];
}

bool InputManager::IsMouseButtonJustReleased(int button) const {
     if (button < 0 || button > 2) return false;
     return !m_mouseButtonState[button] && m_prevMouseButtonState[button];
}

int InputManager::GetMouseWheelDelta() const {
    return m_mouseWheelDelta; // Accumulated in WndProc
}

// --- Raw Mouse Input Handling ---
void InputManager::ProcessRawMouseInput(LPARAM lParam) {
     if (!m_captureMouse) return; // Only process if capturing

     UINT dwSize = 0;
     // First call gets the size of the buffer needed
     GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));

     if (dwSize == 0) return;

     // Allocate buffer dynamically or use a static one if preferred
     std::vector<BYTE> lpb(dwSize);
    // static BYTE lpb[sizeof(RAWINPUT)] = {}; // Example static buffer

     if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
         OutputDebugString(L"GetRawInputData returned incorrect size.\n");
         return;
     }

     RAWINPUT* raw = (RAWINPUT*)lpb.data();

     if (raw->header.dwType == RIM_TYPEMOUSE) {
         // Accumulate deltas for this frame
         m_mouseDelta.x += raw->data.mouse.lLastX;
         m_mouseDelta.y += raw->data.mouse.lLastY;
         m_mouseMovedSinceUpdate = true; // Flag that raw input was processed

         // Process wheel data if needed (often handled via WM_MOUSEWHEEL)
         if (raw->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {
             m_mouseWheelDelta += (short)raw->data.mouse.usButtonData;
         }
     }
}

void InputManager::SetCaptureMouse(bool capture) {
    if (m_captureMouse == capture) return; // No change

    m_captureMouse = capture;
    ShowCursor(!capture); // Hide cursor when capturing

    if (capture) {
        // Center cursor immediately when starting capture
        UpdateMousePosition();
        m_prevMousePos = m_mousePos; // Set prev pos to center
        m_mouseDelta = {0, 0}; // Reset delta
        // Clip cursor to window (optional, but good practice when capturing)
        RECT clipRect;
        GetClientRect(m_hwnd, &clipRect); // Get client area
        ClientToScreen(m_hwnd, (POINT*)&clipRect.left); // Convert top-left to screen coords
        ClientToScreen(m_hwnd, (POINT*)&clipRect.right); // Convert bottom-right to screen coords
        ClipCursor(&clipRect);
    } else {
        ClipCursor(NULL); // Release cursor clipping
    }
}

// --- Gamepad Methods ---
bool InputManager::IsGamepadConnected(int playerIndex) const {
    if (playerIndex < 0 || playerIndex >= XUSER_MAX_COUNT) return false;
    return m_gamepadConnected[playerIndex];
}

const XINPUT_STATE& InputManager::GetGamepadState(int playerIndex) const {
    // Should ideally check if connected before calling, or return a default state
    if (playerIndex < 0 || playerIndex >= XUSER_MAX_COUNT) {
        static XINPUT_STATE dummyState = {}; // Return a zeroed state for invalid index
        return dummyState;
    }
    return m_gamepadState[playerIndex];
}

bool InputManager::IsGamepadButtonDown(int playerIndex, WORD button) const {
    if (!IsGamepadConnected(playerIndex)) return false;
    return (m_gamepadState[playerIndex].Gamepad.wButtons & button) != 0;
}

bool InputManager::IsGamepadButtonJustPressed(int playerIndex, WORD button) const {
    if (!IsGamepadConnected(playerIndex)) return false;
    return ((m_gamepadState[playerIndex].Gamepad.wButtons & button) != 0) &&
           !((m_prevGamepadState[playerIndex].Gamepad.wButtons & button) != 0);
}

bool InputManager::IsGamepadButtonJustReleased(int playerIndex, WORD button) const {
     if (!IsGamepadConnected(playerIndex)) return false;
     return !((m_gamepadState[playerIndex].Gamepad.wButtons & button) != 0) &&
            ((m_prevGamepadState[playerIndex].Gamepad.wButtons & button) != 0);
}

float InputManager::GetGamepadTrigger(int playerIndex, bool isLeftTrigger) const {
    if (!IsGamepadConnected(playerIndex)) return 0.0f;

    BYTE triggerValue = isLeftTrigger ? m_gamepadState[playerIndex].Gamepad.bLeftTrigger
                                      : m_gamepadState[playerIndex].Gamepad.bRightTrigger;

    if (triggerValue < TRIGGER_THRESHOLD) {
        return 0.0f;
    }

    // Normalize to 0.0f - 1.0f range
    return static_cast<float>(triggerValue - TRIGGER_THRESHOLD) / (255.0f - TRIGGER_THRESHOLD);
}

DirectX::XMFLOAT2 InputManager::GetGamepadThumbstick(int playerIndex, bool isLeftStick) const {
    DirectX::XMFLOAT2 stick = { 0.0f, 0.0f };
    if (!IsGamepadConnected(playerIndex)) return stick;

    SHORT deadzone = isLeftStick ? LEFT_THUMB_DEADZONE : RIGHT_THUMB_DEADZONE;
    SHORT stickX = isLeftStick ? m_gamepadState[playerIndex].Gamepad.sThumbLX : m_gamepadState[playerIndex].Gamepad.sThumbRX;
    SHORT stickY = isLeftStick ? m_gamepadState[playerIndex].Gamepad.sThumbLY : m_gamepadState[playerIndex].Gamepad.sThumbRY;

    // Apply deadzone calculation
    float x = static_cast<float>(stickX);
    float y = static_cast<float>(stickY);

    ApplyStickDeadzone(x, y, deadzone);

    // Normalize to -1.0f to 1.0f range
    // Note: Max value is 32767, Min is -32768. Normalization might be slightly asymmetric.
    stick.x = x / 32767.0f;
    stick.y = y / 32767.0f;

    return stick;
}

// Helper function to apply radial deadzone and scaling
void InputManager::ApplyStickDeadzone(float& x, float& y, SHORT deadzone) {
    float magnitude = sqrtf(x*x + y*y);

    if (magnitude < deadzone) {
        x = 0.0f;
        y = 0.0f;
    } else {
        // Optional: Scale input to map range [deadzone, 32767] to [0, 32767]
        // This provides a smoother transition out of the deadzone.
        float normalizedMagnitude = (magnitude - deadzone) / (32767.0f - deadzone);
        normalizedMagnitude = std::min(normalizedMagnitude, 1.0f); // Clamp to 1.0

        float scale = normalizedMagnitude / magnitude; // Ratio to scale original vector
        x *= scale;
        y *= scale;
    }
}
