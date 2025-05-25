// Agrona
// Copyright (c) 2025 CGLJ08. All rights reserved.
// This project includes code derived from Microsoft's MSDN samples. See the LICENSE file for details.

#include "pch.h"
#include "PhysicsManager.h"
#include <limits> // For infinity

using namespace DirectX;

PhysicsManager::PhysicsManager() : m_nextObjectId(0), m_gravity({0.0f, -9.81f, 0.0f}) {}

PhysicsManager::~PhysicsManager() {
    Shutdown();
}

void PhysicsManager::Initialize(DirectX::XMFLOAT3 gravity) {
    m_gravity = gravity;
    m_objects.clear();
    m_projectiles.clear();
    m_nextObjectId = 0;
}

void PhysicsManager::Shutdown() {
    m_objects.clear();
    m_projectiles.clear();
}

int PhysicsManager::AddObject(const PhysicsObject& obj) {
    m_objects.push_back(obj);
    m_objects.back().ObjectId = m_nextObjectId;
    return m_nextObjectId++;
}

void PhysicsManager::RemoveObject(int objectId) {
    m_objects.erase(std::remove_if(m_objects.begin(), m_objects.end(),
        [objectId](const PhysicsObject& obj){ return obj.ObjectId == objectId; }),
        m_objects.end());
    // Note: Removing projectiles associated with this object might be needed
}

PhysicsObject* PhysicsManager::GetObject(int objectId) {
     for (auto& obj : m_objects) {
         if (obj.ObjectId == objectId) {
             return &obj;
         }
     }
     return nullptr;
}


int PhysicsManager::AddProjectile(const Projectile& proj) {
    m_projectiles.push_back(proj);
    m_projectiles.back().ObjectId = m_nextObjectId; // Use same ID pool for simplicity
    return m_nextObjectId++;
}

void PhysicsManager::RemoveProjectile(int objectId) {
     m_projectiles.erase(std::remove_if(m_projectiles.begin(), m_projectiles.end(),
         [objectId](const Projectile& p){ return p.ObjectId == objectId; }),
         m_projectiles.end());
}

Projectile* PhysicsManager::GetProjectile(int objectId) {
      for (auto& proj : m_projectiles) {
          if (proj.ObjectId == objectId) {
              return &proj;
          }
      }
      return nullptr;
}


void PhysicsManager::Update(float deltaTime) {
    if (deltaTime <= 0.0f) return; // Avoid issues with zero or negative delta time

    XMVECTOR gravityVec = XMLoadFloat3(&m_gravity);

    // --- Update Physics Objects ---
    for (auto& obj : m_objects) {
        if (obj.IsStatic) continue;

        // Apply gravity force
        if (obj.HasGravity) {
            obj.Acceleration.y += m_gravity.y; // Simple gravity application
            // F = m*a -> a = F/m. If using forces directly:
            // XMVECTOR force = XMVectorScale(gravityVec, obj.Mass);
            // ApplyForce(obj.ObjectId, force); // Use ApplyForce method
        }

        // Integrate velocity (Velocity += Acceleration * dt)
        XMVECTOR vel = XMLoadFloat3(&obj.Velocity);
        XMVECTOR acc = XMLoadFloat3(&obj.Acceleration);
        vel = XMVectorAdd(vel, XMVectorScale(acc, deltaTime));
        XMStoreFloat3(&obj.Velocity, vel);

        // Integrate position (Position += Velocity * dt)
        XMVECTOR pos = XMLoadFloat3(&obj.Position);
        pos = XMVectorAdd(pos, XMVectorScale(vel, deltaTime));
        XMStoreFloat3(&obj.Position, pos);

        // Reset acceleration for next frame (forces are typically applied each frame)
        obj.Acceleration = {0, 0, 0};

        // TODO: Add damping/friction
        // XMVECTOR damping = XMVectorScale(vel, -DragCoefficient * deltaTime);
        // vel = XMVectorAdd(vel, damping);
        // XMStoreFloat3(&obj.Velocity, vel);
    }

    // --- Update Projectiles ---
    std::vector<int> projectilesToRemove;
    for (auto& proj : m_projectiles) {
        // Apply gravity (projectiles usually have gravity)
         if (proj.HasGravity) {
             proj.Acceleration.y += m_gravity.y;
         }

        // Integrate velocity
        XMVECTOR vel = XMLoadFloat3(&proj.Velocity);
        XMVECTOR acc = XMLoadFloat3(&proj.Acceleration);
        vel = XMVectorAdd(vel, XMVectorScale(acc, deltaTime));
        XMStoreFloat3(&proj.Velocity, vel);

        // Integrate position
        XMVECTOR pos = XMLoadFloat3(&proj.Position);
        pos = XMVectorAdd(pos, XMVectorScale(vel, deltaTime));
        XMStoreFloat3(&proj.Position, pos);

        // Reset acceleration
        proj.Acceleration = {0, 0, 0};

        // Decrease lifetime
        proj.Lifetime -= deltaTime;
        if (proj.Lifetime <= 0.0f) {
            projectilesToRemove.push_back(proj.ObjectId);
            continue; // Don't check collisions for projectiles being removed this frame
        }

        // --- Basic Projectile Collision Check ---
        AABB projWorldBox = GetWorldAABB(proj);
        for (const auto& obj : m_objects) {
            if (proj.ObjectId == obj.ObjectId) continue; // Don't collide with self (if obj can be proj)

            AABB objWorldBox = GetWorldAABB(obj);
            if (projWorldBox.Intersects(objWorldBox)) {
                // Collision detected!
                OutputDebugStringA(("Projectile collision: Proj " + std::to_string(proj.ObjectId) + " hit Obj " + std::to_string(obj.ObjectId) + "\n").c_str());

                // TODO: Handle collision response (e.g., damage object, play sound, create effect)

                projectilesToRemove.push_back(proj.ObjectId); // Remove projectile on hit
                break; // Projectile hit something, stop checking for this projectile
            }
        }
         // TODO: Add projectile-projectile collision if needed
    }

    // Remove expired or collided projectiles
    for (int id : projectilesToRemove) {
        RemoveProjectile(id);
    }


    // --- General Object Collision Detection & Response ---
    // Naive N^2 check - VERY slow for many objects. Use broadphase (e.g., grid, BVH) for optimization.
    for (size_t i = 0; i < m_objects.size(); ++i) {
         if (m_objects[i].IsStatic) continue; // Static objects don't initiate collision checks here

         AABB box_i = GetWorldAABB(m_objects[i]);

         for (size_t j = i + 1; j < m_objects.size(); ++j) {
             AABB box_j = GetWorldAABB(m_objects[j]);

             if (box_i.Intersects(box_j)) {
                 // Collision detected between object i and object j
                 OutputDebugStringA(("Object collision: Obj " + std::to_string(m_objects[i].ObjectId) + " hit Obj " + std::to_string(m_objects[j].ObjectId) + "\n").c_str());

                 // Basic collision response
                 ResolveCollision(m_objects[i], m_objects[j]);
             }
         }
    }

     // TODO: Terrain Collision (e.g., using heightmap lookups or raycasts downwards)

}


// Basic AABB check
bool PhysicsManager::CheckCollision(int objectIdA, int objectIdB) {
    PhysicsObject* objA = GetObject(objectIdA);
    PhysicsObject* objB = GetObject(objectIdB);
    if (!objA || !objB) return false;

    AABB worldA = GetWorldAABB(*objA);
    AABB worldB = GetWorldAABB(*objB);

    return worldA.Intersects(worldB);
}

// Very basic AABB Raycast - doesn't return closest hit, just first found
bool PhysicsManager::Raycast(const Ray& ray, float maxDistance, int& outHitObjectId, XMFLOAT3& outHitPoint) {
     XMVECTOR rayOrigin = XMLoadFloat3(&ray.Origin);
     XMVECTOR rayDir = XMLoadFloat3(&ray.Direction); // Assumed normalized

     float closestHitDistSq = maxDistance * maxDistance;
     bool hitFound = false;
     outHitObjectId = -1;

     for (const auto& obj : m_objects) {
         AABB worldBox = GetWorldAABB(obj);
         float dist; // Distance parameter along the ray

         // DirectXCollision has Ray-AABB intersection tests (BoundingBox::Intersects)
         // Manual Slab Test:
         float tmin = 0.0f;
         float tmax = std::numeric_limits<float>::infinity();

         XMVECTOR boxMin = XMLoadFloat3(&worldBox.Min);
         XMVECTOR boxMax = XMLoadFloat3(&worldBox.Max);

         // Check intersection with slabs for each axis
         for (int axis = 0; axis < 3; ++axis) {
             float dirAxis = XMVectorGetByIndex(rayDir, axis);
             float originAxis = XMVectorGetByIndex(rayOrigin, axis);
             float minAxis = XMVectorGetByIndex(boxMin, axis);
             float maxAxis = XMVectorGetByIndex(boxMax, axis);

             if (fabsf(dirAxis) < 1e-6f) { // Ray parallel to slab
                 if (originAxis < minAxis || originAxis > maxAxis) {
                     tmax = -1.0f; // Missed
                     break;
                 }
             } else {
                 float t1 = (minAxis - originAxis) / dirAxis;
                 float t2 = (maxAxis - originAxis) / dirAxis;

                 if (t1 > t2) std::swap(t1, t2); // Ensure t1 is entry, t2 is exit

                 tmin = std::max(tmin, t1);
                 tmax = std::min(tmax, t2);

                 if (tmin > tmax) { // Box is missed
                      tmax = -1.0f;
                      break;
                 }
             }
         }

         if (tmax >= 0 && tmin <= tmax && (tmin * tmin) < closestHitDistSq) { // Check if hit is within range and closer
             float hitDist = (tmin < 0.0f) ? tmax : tmin; // Use entry point if possible
             if (hitDist >= 0 && (hitDist*hitDist) < closestHitDistSq) {
                  closestHitDistSq = hitDist * hitDist;
                  outHitObjectId = obj.ObjectId;
                  XMStoreFloat3(&outHitPoint, XMVectorAdd(rayOrigin, XMVectorScale(rayDir, hitDist)));
                  hitFound = true;
             }
         }
     }

    return hitFound;
}


void PhysicsManager::ApplyForce(int objectId, const XMFLOAT3& force) {
    PhysicsObject* obj = GetObject(objectId);
    if (obj && !obj->IsStatic && obj->Mass > 0.0f) {
        XMVECTOR acc = XMLoadFloat3(&obj->Acceleration);
        XMVECTOR f = XMLoadFloat3(&force);
        // a = F/m
        acc = XMVectorAdd(acc, XMVectorScale(f, 1.0f / obj->Mass));
        XMStoreFloat3(&obj->Acceleration, acc);
    }
     // Apply to projectiles too?
     Projectile* proj = GetProjectile(objectId);
      if (proj && !proj->IsStatic && proj->Mass > 0.0f) {
         XMVECTOR acc = XMLoadFloat3(&proj->Acceleration);
         XMVECTOR f = XMLoadFloat3(&force);
         acc = XMVectorAdd(acc, XMVectorScale(f, 1.0f / proj->Mass));
         XMStoreFloat3(&proj->Acceleration, acc);
     }
}

void PhysicsManager::ApplyImpulse(int objectId, const XMFLOAT3& impulse) {
    PhysicsObject* obj = GetObject(objectId);
    if (obj && !obj->IsStatic && obj->Mass > 0.0f) {
        XMVECTOR vel = XMLoadFloat3(&obj->Velocity);
        XMVECTOR imp = XMLoadFloat3(&impulse);
        // deltaV = Impulse / mass
        vel = XMVectorAdd(vel, XMVectorScale(imp, 1.0f / obj->Mass));
        XMStoreFloat3(&obj->Velocity, vel);
    }
     // Apply to projectiles too?
     Projectile* proj = GetProjectile(objectId);
     if (proj && !proj->IsStatic && proj->Mass > 0.0f) {
         XMVECTOR vel = XMLoadFloat3(&proj->Velocity);
         XMVECTOR imp = XMLoadFloat3(&impulse);
         vel = XMVectorAdd(vel, XMVectorScale(imp, 1.0f / proj->Mass));
         XMStoreFloat3(&proj->Velocity, vel);
     }
}

// Helper to get world-space AABB (assumes no rotation for now!)
AABB PhysicsManager::GetWorldAABB(const PhysicsObject& obj) const {
    AABB worldBox = obj.BoundingBox;
    // If object has rotation, this needs to transform all 8 corners
    // and find the new min/max, which is more complex.
    worldBox.Min.x += obj.Position.x; worldBox.Min.y += obj.Position.y; worldBox.Min.z += obj.Position.z;
    worldBox.Max.x += obj.Position.x; worldBox.Max.y += obj.Position.y; worldBox.Max.z += obj.Position.z;
    return worldBox;
}


// Very simple collision response: Push objects apart based on overlap
void PhysicsManager::ResolveCollision(PhysicsObject& objA, PhysicsObject& objB) {
    if (objA.IsStatic && objB.IsStatic) return; // Neither can move

    AABB worldA = GetWorldAABB(objA);
    AABB worldB = GetWorldAABB(objB);

    // Calculate overlap on each axis
    float overlapX = std::min(worldA.Max.x, worldB.Max.x) - std::max(worldA.Min.x, worldB.Min.x);
    float overlapY = std::min(worldA.Max.y, worldB.Max.y) - std::max(worldA.Min.y, worldB.Min.y);
    float overlapZ = std::min(worldA.Max.z, worldB.Max.z) - std::max(worldA.Min.z, worldB.Min.z);

    // Find axis of minimum overlap (potential collision normal direction)
    XMFLOAT3 push = {0,0,0};
    float minOverlap = std::numeric_limits<float>::infinity();

    if (overlapX < minOverlap) { minOverlap = overlapX; push = {(objA.Position.x < objB.Position.x) ? -overlapX : overlapX, 0, 0}; }
    if (overlapY < minOverlap) { minOverlap = overlapY; push = {0, (objA.Position.y < objB.Position.y) ? -overlapY : overlapY, 0}; }
    if (overlapZ < minOverlap) { minOverlap = overlapZ; push = {0, 0, (objA.Position.z < objB.Position.z) ? -overlapZ : overlapZ}; }


    // Calculate total inverse mass (static objects have infinite mass -> inverse mass = 0)
    float invMassA = objA.IsStatic ? 0.0f : 1.0f / objA.Mass;
    float invMassB = objB.IsStatic ? 0.0f : 1.0f / objB.Mass;
    float totalInvMass = invMassA + invMassB;

    if (totalInvMass <= 0.0f) return; // Both are static or have zero/negative mass

    // Distribute the push based on inverse mass ratio (lighter objects move more)
     float correctionFactor = 0.8f; // Factor less than 1 to avoid jitter / sinking
     XMVECTOR pushVec = XMVectorScale(XMLoadFloat3(&push), correctionFactor / totalInvMass);

    // Apply position correction
    if (!objA.IsStatic) {
        XMVECTOR posA = XMLoadFloat3(&objA.Position);
        posA = XMVectorAdd(posA, XMVectorScale(pushVec, invMassA));
        XMStoreFloat3(&objA.Position, posA);
    }
    if (!objB.IsStatic) {
         XMVECTOR posB = XMLoadFloat3(&objB.Position);
         posB = XMVectorSubtract(posB, XMVectorScale(pushVec, invMassB)); // Push B in opposite direction
         XMStoreFloat3(&objB.Position, posB);
    }

    // TODO: Implement impulse-based response for velocities
    // Calculate relative velocity, collision normal, apply impulse based on restitution etc.
}
