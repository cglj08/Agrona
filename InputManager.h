// Agrona
// Copyright (c) 2025 CGLJ08. All rights reserved.
// This project includes code derived from Microsoft's MSDN samples. See the LICENSE file for details.

#pragma once

#include "pch.h" // Includes XInput.h, Windows.h

class InputManager {
public:
    InputManager();
    ~InputManager();

    bool Initialize(HWND hwnd);
    void Shutdown();
    void Update(); // Call once per frame BEFORE game logic

    // Keyboard
    bool IsKeyDown(int vkCode) const;
    bool IsKeyJustPressed(int vkCode) const;
    bool IsKeyJustReleased(int vkCode) const;

    // Mouse
    POINT GetMousePosition() const;
    POINT GetMouseDelta() const; // Change in position since last frame
    bool IsMouseButtonDown(int button) const; // 0=Left, 1=Right, 2=Middle
    bool IsMouseButtonJustPressed(int button) const;
    bool IsMouseButtonJustReleased(int button) const;
    int GetMouseWheelDelta() const; // Accumulated wheel delta since last Update()

    // Gamepad (XInput)
    bool IsGamepadConnected(int playerIndex) const; // 0-3
    const XINPUT_STATE& GetGamepadState(int playerIndex) const;
    bool IsGamepadButtonDown(int playerIndex, WORD button) const; // e.g., XINPUT_GAMEPAD_A
    bool IsGamepadButtonJustPressed(int playerIndex, WORD button) const;
    bool IsGamepadButtonJustReleased(int playerIndex, WORD button) const;
    float GetGamepadTrigger(int playerIndex, bool isLeftTrigger) const; // 0.0f to 1.0f
    DirectX::XMFLOAT2 GetGamepadThumbstick(int playerIndex, bool isLeftStick) const; // -1.0f to 1.0f for X and Y

    // Raw Mouse Input Handling (for smoother camera control) - needs setup in WndProc
    void ProcessRawMouseInput(LPARAM lParam);
    void SetCaptureMouse(bool capture); // To hide cursor and keep mouse centered

private:
    HWND m_hwnd = nullptr;

    // Keyboard State
    BYTE m_keyboardState[256];
    BYTE m_prevKeyboardState[256];

    // Mouse State
    POINT m_mousePos = {0, 0};
    POINT m_prevMousePos = {0, 0};
    POINT m_mouseDelta = {0, 0};
    bool m_mouseButtonState[3];      // L, R, M
    bool m_prevMouseButtonState[3];
    int m_mouseWheelDelta = 0;
    bool m_captureMouse = false;
    bool m_mouseMovedSinceUpdate = false;


    // Gamepad State
    XINPUT_STATE m_gamepadState[XUSER_MAX_COUNT];       // Current state
    XINPUT_STATE m_prevGamepadState[XUSER_MAX_COUNT]; // Previous frame state
    bool m_gamepadConnected[XUSER_MAX_COUNT];

    // Deadzone constants
    static constexpr SHORT LEFT_THUMB_DEADZONE = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    static constexpr SHORT RIGHT_THUMB_DEADZONE = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
    static constexpr BYTE TRIGGER_THRESHOLD = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

    void UpdateMousePosition();
    void ApplyStickDeadzone(float& x, float& y, SHORT deadzone);
};

