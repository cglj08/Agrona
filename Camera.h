// Agrona
// Copyright (c) 2025 CGLJ08. All rights reserved.
// This project includes code derived from Microsoft's MSDN samples. See the LICENSE file for details.

#pragma once

#include "pch.h"

enum class CameraMode {
    FPS,
    Follow
};

class InputManager; // Forward declaration

class Camera {
public:
    Camera();

    void SetMode(CameraMode mode);
    CameraMode GetMode() const;

    // Set initial position and orientation (FPS) or target (Follow)
    void SetPosition(float x, float y, float z);
    void LookAt(const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& worldUp = { 0.0f, 1.0f, 0.0f });
    void SetTarget(const DirectX::XMFLOAT3& targetPosition, float distance, float heightOffset); // For Follow cam

    // Update projection matrix (call on resize)
    void UpdateProjectionMatrix(float fovAngleY, float aspectRatio, float nearZ, float farZ);

    // Update view matrix (call each frame BEFORE getting matrices)
    void Update(float deltaTime, const InputManager& input, const DirectX::XMFLOAT3* followTargetPos = nullptr);

    // Get matrices for rendering
    DirectX::XMMATRIX GetViewMatrix() const;
    DirectX::XMMATRIX GetProjectionMatrix() const;
    DirectX::XMFLOAT4X4 GetViewMatrixFloat4x4() const;
    DirectX::XMFLOAT4X4 GetProjectionMatrixFloat4x4() const;
    DirectX::XMFLOAT3 GetPosition() const;
    DirectX::XMFLOAT3 GetLookDirection() const;


    // Camera Movement Parameters (tune these)
    float MoveSpeed = 10.0f;
    float LookSensitivity = 0.003f; // Radians per pixel delta
    float FollowDistance = 10.0f;
    float FollowHeightOffset = 2.0f;
    float FollowRotationSpeed = 2.0f; // Radians per second for smoothing


private:
    CameraMode m_mode = CameraMode::FPS;

    // Core vectors
    DirectX::XMFLOAT3 m_position = { 0.0f, 0.0f, -5.0f };
    DirectX::XMFLOAT3 m_lookDirection = { 0.0f, 0.0f, 1.0f }; // Forward
    DirectX::XMFLOAT3 m_upDirection = { 0.0f, 1.0f, 0.0f };   // Local Up
    DirectX::XMFLOAT3 m_rightDirection = { 1.0f, 0.0f, 0.0f }; // Local Right

    // Follow camera specific
    DirectX::XMFLOAT3 m_followTargetPos = { 0.0f, 0.0f, 0.0f };
    DirectX::XMFLOAT3 m_currentFollowOffset; // Actual offset used for smoothing

    // Orientation (using Euler angles for simplicity here, consider Quaternions for robustness)
    float m_yaw = DirectX::XM_PIDIV2;   // Rotation around Y (left/right look) - Initialized looking forward along +Z
    float m_pitch = 0.0f; // Rotation around X (up/down look)

    // Matrices
    DirectX::XMFLOAT4X4 m_viewMatrix;
    DirectX::XMFLOAT4X4 m_projectionMatrix;

    // Update helpers
    void UpdateViewMatrixFPS();
    void UpdateViewMatrixFollow(float deltaTime, const DirectX::XMFLOAT3* targetPos);
    void Rotate(float dx, float dy); // Applies mouse delta to yaw/pitch
    void Move(const DirectX::XMFLOAT3& direction, float amount);
};
