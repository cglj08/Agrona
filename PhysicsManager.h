
#pragma once

#include "pch.h"
#include <vector>

// Basic Axis-Aligned Bounding Box
struct AABB {
    DirectX::XMFLOAT3 Min;
    DirectX::XMFLOAT3 Max;

    // Check intersection with another AABB
    bool Intersects(const AABB& other) const {
        return (Max.x >= other.Min.x && Min.x <= other.Max.x) &&
               (Max.y >= other.Min.y && Min.y <= other.Max.y) &&
               (Max.z >= other.Min.z && Min.z <= other.Max.z);
    }
};

// Represents an object in the physics world
struct PhysicsObject {
    int ObjectId = -1; // Link back to game object/entity if needed
    DirectX::XMFLOAT3 Position = {0, 0, 0};
    DirectX::XMFLOAT3 Velocity = {0, 0, 0};
    DirectX::XMFLOAT3 Acceleration = {0, 0, 0};
    AABB BoundingBox; // Local space AABB (needs transform for world space checks)
    float Mass = 1.0f;
    bool IsStatic = false; // Doesn't move or respond to forces
    bool HasGravity = true;
    // Add friction, restitution (bounciness), etc.
};

// Represents a projectile
struct Projectile : PhysicsObject {
    float Lifetime = 5.0f; // Time before disappearing
    // Add damage, owner ID, etc.
};

// Ray structure for collision checks
struct Ray {
    DirectX::XMFLOAT3 Origin;
    DirectX::XMFLOAT3 Direction; // Should be normalized
};

class PhysicsManager {
public:
    PhysicsManager();
    ~PhysicsManager();

    void Initialize(DirectX::XMFLOAT3 gravity = {0.0f, -9.81f, 0.0f});
    void Shutdown();

    // Add/Remove objects
    int AddObject(const PhysicsObject& obj); // Returns ObjectId
    void RemoveObject(int objectId);
    PhysicsObject* GetObject(int objectId);

    int AddProjectile(const Projectile& proj); // Returns ObjectId
    void RemoveProjectile(int objectId);
    Projectile* GetProjectile(int objectId);

    // Update physics simulation
    void Update(float deltaTime);

    // Collision Detection (Basic)
    bool CheckCollision(int objectIdA, int objectIdB); // AABB check
    bool Raycast(const Ray& ray, float maxDistance, int& outHitObjectId, DirectX::XMFLOAT3& outHitPoint); // Basic raycast

    // Apply forces
    void ApplyForce(int objectId, const DirectX::XMFLOAT3& force);
    void ApplyImpulse(int objectId, const DirectX::XMFLOAT3& impulse); // Instant change in velocity


private:
    std::vector<PhysicsObject> m_objects;
    std::vector<Projectile> m_projectiles;
    int m_nextObjectId = 0;
    DirectX::XMFLOAT3 m_gravity;

    // Helper for world space AABB
    AABB GetWorldAABB(const PhysicsObject& obj) const;

    // Simple collision response placeholder
    void ResolveCollision(PhysicsObject& objA, PhysicsObject& objB);
};
