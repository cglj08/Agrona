
#pragma once

#include <vector>
#include <string>
#include <map>
#include <directxmath.h>

// Basic Vertex structure - Customize as needed (e.g., add tangent, bitangent)
struct Vertex {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 TexCoord;
    DirectX::XMFLOAT4 BoneWeights = { 0.0f, 0.0f, 0.0f, 0.0f }; // Example: Max 4 bone influences
    DirectX::XMUINT4 BoneIndices = { 0, 0, 0, 0 };
};

// Dual Quaternion structure (for skinning)
struct DualQuaternion {
    DirectX::XMFLOAT4 Real; // Real part (rotation quaternion)
    DirectX::XMFLOAT4 Dual; // Dual part (translation encoded)
};

struct Material {
    std::wstring Name;
    DirectX::XMFLOAT4 DiffuseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    DirectX::XMFLOAT4 SpecularColor = { 1.0f, 1.0f, 1.0f, 1.0f };
    float SpecularPower = 32.0f;
    std::wstring DiffuseTexturePath;
    // Add paths for normal maps, specular maps etc.
    // ComPtr<ID3D11ShaderResourceView> pDiffuseTextureView; // Loaded texture
};

struct Mesh {
    std::vector<Vertex> Vertices;
    std::vector<uint32_t> Indices;
    std::wstring MaterialName; // Link to a material
    // D3D Buffers - To be created after loading
    Microsoft::WRL::ComPtr<ID3D11Buffer> pVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> pIndexBuffer;
    UINT IndexCount = 0;
    UINT VertexStride = sizeof(Vertex);
    UINT VertexOffset = 0;
};

// Represents a joint in the skeleton
struct Joint {
    std::wstring Name;
    int ParentIndex = -1; // Index of the parent joint in the skeleton's joint list, -1 for root
    DirectX::XMFLOAT4X4 InverseBindPoseMatrix; // Matrix to transform vertex from model space to joint space
    DirectX::XMFLOAT4X4 LocalBindTransform;    // Transform relative to parent in bind pose
    // Add transform for animation:
    DirectX::XMFLOAT3 Translation = {0,0,0};
    DirectX::XMFLOAT4 RotationQuat = {0,0,0,1}; // Rotation as Quaternion
    DirectX::XMFLOAT3 Scale = {1,1,1};
    // Or use Dual Quaternions for animation:
    // DualQuaternion AnimationDQ;
};

struct Skeleton {
    std::vector<Joint> Joints;
    std::map<std::wstring, int> JointNameToIndex; // Quick lookup
};

// Animation keyframes for a single joint/node
struct AnimationChannel {
    std::wstring TargetNodeName; // Name of the Joint or Node being animated
    std::vector<float> PositionTimestamps;
    std::vector<DirectX::XMFLOAT3> Positions;
    std::vector<float> RotationTimestamps;
    std::vector<DirectX::XMFLOAT4> Rotations; // Quaternions
    std::vector<float> ScaleTimestamps;
    std::vector<DirectX::XMFLOAT3> Scales;
    // Add Dual Quaternion keys if using DQ skinning
    // std::vector<float> DQTimestamps;
    // std::vector<DualQuaternion> DQs;
};

struct AnimationClip {
    std::wstring Name;
    float Duration = 0.0f; // Duration in seconds (or ticks, need consistency)
    float TicksPerSecond = 24.0f; // Default, should be read from file
    std::vector<AnimationChannel> Channels;
};

// Represents a loaded model potentially with multiple meshes and a skeleton
struct Model {
    std::vector<Mesh> Meshes;
    std::vector<Material> Materials; // Materials used by meshes in this model
    std::map<std::wstring, int> MaterialNameToIndex;
    std::unique_ptr<Skeleton> pSkeleton = nullptr; // Optional skeleton
    std::vector<AnimationClip> Animations; // Optional animations
    // Add bounding box, etc.
};

