
#include "pch.h"
#include "Camera.h"
#include "InputManager.h" // Include full header for input checks

using namespace DirectX;

Camera::Camera() {
    // Initialize matrices to identity
    XMStoreFloat4x4(&m_viewMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&m_projectionMatrix, XMMatrixIdentity());
    m_currentFollowOffset = {0.0f, FollowHeightOffset, -FollowDistance}; // Initial offset behind
}

void Camera::SetMode(CameraMode mode) {
    m_mode = mode;
    // Reset orientation or target offset if needed when switching modes
    if (mode == CameraMode::FPS) {
         // Recalculate direction vectors based on current yaw/pitch
         Rotate(0,0); // Force update of direction vectors
    } else {
         // Initialize follow offset based on current distance/height settings
         m_currentFollowOffset = {0.0f, FollowHeightOffset, -FollowDistance};
    }
}

CameraMode Camera::GetMode() const {
    return m_mode;
}

void Camera::SetPosition(float x, float y, float z) {
    m_position = { x, y, z };
}

void Camera::LookAt(const XMFLOAT3& target, const XMFLOAT3& worldUp) {
    XMVECTOR P = XMLoadFloat3(&m_position);
    XMVECTOR T = XMLoadFloat3(&target);
    XMVECTOR U = XMLoadFloat3(&worldUp);

    XMMATRIX V = XMMatrixLookAtLH(P, T, U); // Use LH or RH based on your convention
    XMStoreFloat4x4(&m_viewMatrix, V);

    // Update internal direction vectors and orientation from the view matrix
    XMFLOAT4X4 viewInv;
    XMStoreFloat4x4(&viewInv, XMMatrixInverse(nullptr, V));

    m_rightDirection = { viewInv._11, viewInv._12, viewInv._13 };
    m_upDirection = { viewInv._21, viewInv._22, viewInv._23 };
    m_lookDirection = { viewInv._31, viewInv._32, viewInv._33 };

    // Approximate yaw and pitch (can be tricky to get exact angles back)
    m_pitch = asinf(-viewInv._32); // asin(-m32)
    // Check for singularity at poles
    if (fabsf(cosf(m_pitch)) > 0.0001f) {
        m_yaw = atan2f(viewInv._31, viewInv._33); // atan2(m31, m33)
    } else {
        // At pole, yaw can be derived differently, e.g., from right vector
        m_yaw = atan2f(-viewInv._13, viewInv._11); // atan2(-m13, m11)
    }
}

void Camera::SetTarget(const XMFLOAT3& targetPosition, float distance, float heightOffset) {
     m_followTargetPos = targetPosition;
     FollowDistance = distance;
     FollowHeightOffset = heightOffset;
     // Update the desired offset immediately (smoothing will apply over time in Update)
     m_currentFollowOffset = {0.0f, heightOffset, -distance};
}

void Camera::UpdateProjectionMatrix(float fovAngleY, float aspectRatio, float nearZ, float farZ) {
    XMMATRIX P = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, nearZ, farZ); // Use LH or RH
    XMStoreFloat4x4(&m_projectionMatrix, P);
}

void Camera::Update(float deltaTime, const InputManager& input, const XMFLOAT3* followTargetPos) {
    if (m_mode == CameraMode::FPS) {
        // --- FPS Camera Update ---

        // Mouse Look
        POINT mouseDelta = input.GetMouseDelta();
        if (mouseDelta.x != 0 || mouseDelta.y != 0) {
            Rotate(static_cast<float>(mouseDelta.x), static_cast<float>(mouseDelta.y));
        }

        // Keyboard Movement
        XMFLOAT3 moveDir = {0, 0, 0};
        if (input.IsKeyDown('W')) {
            moveDir.z += 1.0f;
        }
        if (input.IsKeyDown('S')) {
            moveDir.z -= 1.0f;
        }
        if (input.IsKeyDown('A')) {
            moveDir.x -= 1.0f;
        }
        if (input.IsKeyDown('D')) {
            moveDir.x += 1.0f;
        }
        if (input.IsKeyDown(VK_SPACE)) { // Or 'E'/'Q' for up/down
             moveDir.y += 1.0f;
        }
        if (input.IsKeyDown(VK_CONTROL)) { // Or 'C'
             moveDir.y -= 1.0f;
        }

        // Normalize move direction if diagonal
        XMVECTOR moveVec = XMLoadFloat3(&moveDir);
        if (XMVectorGetX(XMVector3LengthSq(moveVec)) > 0.0f) { // Check length squared is > 0
             moveVec = XMVector3Normalize(moveVec);
             XMStoreFloat3(&moveDir, moveVec);
             Move(moveDir, MoveSpeed * deltaTime);
        }

        UpdateViewMatrixFPS();

    } else { // CameraMode::Follow
        // --- Follow Camera Update ---
        if (followTargetPos) {
             m_followTargetPos = *followTargetPos; // Update target position
        }
        // Smoothly interpolate camera position towards target offset
        UpdateViewMatrixFollow(deltaTime, followTargetPos);
    }
}


void Camera::Rotate(float dx, float dy) {
    m_yaw += dx * LookSensitivity;
    m_pitch += dy * LookSensitivity;

    // Clamp pitch to prevent flipping upside down
    m_pitch = std::max(-XM_PIDIV2 + 0.01f, std::min(XM_PIDIV2 - 0.01f, m_pitch));

    // Keep yaw between 0 and 2*PI (optional, avoids large numbers)
    m_yaw = fmodf(m_yaw, XM_2PI);
    if (m_yaw < 0.0f) {
        m_yaw += XM_2PI;
    }

    // Recalculate direction vectors based on new yaw/pitch (FPS mode)
    float cosPitch = cosf(m_pitch);
    float sinPitch = sinf(m_pitch);
    float cosYaw = cosf(m_yaw);
    float sinYaw = sinf(m_yaw);

    m_lookDirection = { cosPitch * sinYaw, -sinPitch, cosPitch * cosYaw };
    // Normalize just in case
    XMVECTOR L = XMVector3Normalize(XMLoadFloat3(&m_lookDirection));
    XMStoreFloat3(&m_lookDirection, L);

    // Create Right vector from Look and World Up (assuming world up is 0,1,0)
    XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L)); // R = Up x Look
    XMStoreFloat3(&m_rightDirection, R);

    // Create orthogonal Up vector
    XMVECTOR U = XMVector3Normalize(XMVector3Cross(L, R)); // U = Look x Right
    XMStoreFloat3(&m_upDirection, U);
}


void Camera::Move(const XMFLOAT3& direction, float amount) {
    XMVECTOR P = XMLoadFloat3(&m_position);
    XMVECTOR L = XMLoadFloat3(&m_lookDirection);
    XMVECTOR R = XMLoadFloat3(&m_rightDirection);
    // Note: For FPS style, moving 'up' usually means along the world Y axis,
    // unless you want flight simulator style controls where 'up' is relative to the camera.
    XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    // P += direction.z * L * amount; // Move along Look direction
    // P += direction.x * R * amount; // Move along Right direction
    // P += direction.y * worldUp * amount; // Move along World Up direction

    // Optimized calculation
     XMVECTOR displacement = XMVectorScale(L, direction.z);
     displacement = XMVectorAdd(displacement, XMVectorScale(R, direction.x));
     displacement = XMVectorAdd(displacement, XMVectorScale(worldUp, direction.y)); // Use world up for vertical

     P = XMVectorAdd(P, XMVectorScale(displacement, amount));

    XMStoreFloat3(&m_position, P);
}


void Camera::UpdateViewMatrixFPS() {
    XMVECTOR P = XMLoadFloat3(&m_position);
    XMVECTOR L = XMLoadFloat3(&m_lookDirection);
    XMVECTOR U = XMLoadFloat3(&m_upDirection); // Use camera's local up

    // Target point is position + look direction
    XMVECTOR T = XMVectorAdd(P, L);

    XMMATRIX V = XMMatrixLookAtLH(P, T, U); // Use LH or RH
    XMStoreFloat4x4(&m_viewMatrix, V);
}

void Camera::UpdateViewMatrixFollow(float deltaTime, const DirectX::XMFLOAT3* targetPos) {
     if (!targetPos) {
          // If no target provided, just use the last known target and don't move
          UpdateViewMatrixFPS(); // Or just return / use previous matrix
          return;
     }

     XMVECTOR currentTargetPos = XMLoadFloat3(targetPos);

     // Ideal camera position calculation (target + offset)
     // For simplicity, assume offset is fixed relative to world axes for now.
     // A better follow cam rotates the offset based on target orientation or player input.
     XMVECTOR desiredOffset = XMVectorSet(0.0f, FollowHeightOffset, -FollowDistance, 0.0f); // Simple offset behind and above

     // --- TODO: Advanced Follow Cam Logic ---
     // Rotate desiredOffset based on target's rotation or player input (right stick?)
     // Example: Get target's forward vector, create rotation matrix, transform offset.
     // ---

     XMVECTOR desiredPos = XMVectorAdd(currentTargetPos, desiredOffset);

     // Smoothly interpolate current position towards desired position
     // Using Lerp for simplicity. Can use spring dynamics for better feel.
     XMVECTOR currentPos = XMLoadFloat3(&m_position);
     XMVECTOR interpPos = XMVectorLerp(currentPos, desiredPos, std::min(FollowRotationSpeed * deltaTime, 1.0f)); // Clamp factor to 1.0

     XMStoreFloat3(&m_position, interpPos);

     // Look at the target (slightly above the base position usually feels better)
     XMFLOAT3 lookAtTarget = *targetPos;
     lookAtTarget.y += FollowHeightOffset * 0.5f; // Aim slightly above the target's base

     LookAt(lookAtTarget, {0.0f, 1.0f, 0.0f}); // Update view matrix based on new position and look-at
}


// --- Getters ---
XMMATRIX Camera::GetViewMatrix() const {
    return XMLoadFloat4x4(&m_viewMatrix);
}

XMMATRIX Camera::GetProjectionMatrix() const {
    return XMLoadFloat4x4(&m_projectionMatrix);
}

XMFLOAT4X4 Camera::GetViewMatrixFloat4x4() const {
    return m_viewMatrix;
}

XMFLOAT4X4 Camera::GetProjectionMatrixFloat4x4() const {
    return m_projectionMatrix;
}

XMFLOAT3 Camera::GetPosition() const {
    return m_position;
}

XMFLOAT3 Camera::GetLookDirection() const {
     return m_lookDirection;
}
