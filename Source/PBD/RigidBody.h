#pragma once

namespace PBD
{
    struct RigidBody
    {
        DirectX::XMFLOAT3 position{ 0,0,0 };
        DirectX::XMFLOAT4 rotation{ 0,0,0,1 }; // quaternion

        DirectX::XMFLOAT3 linearVelocity{ 0,0,0 };
        DirectX::XMFLOAT3 angularVelocity{ 0,0,0 }; // ワールド空間の角速度ベクトル

        float mass = 1.0f;
        float inverseMass = 1.0f;

        // ここでは等方的な慣性モーメントで簡略化
        float inertia = 1.0f;
        float inverseInertia = 1.0f;
    };


    void IntegrateRigidBody(RigidBody& rb, float dt);

    void SolvePlaneForRigidBody(PBD::RigidBody& rb, float planeY, float radius);

    bool SolveBoxForRigidBody(PBD::RigidBody& rb, const DirectX::XMFLOAT3& boxMin, const DirectX::XMFLOAT3& boxMax, float radius);

    void ComputeAABBFromOBB(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& extent, const DirectX::XMFLOAT3X3& rot, DirectX::XMFLOAT3& outMin, DirectX::XMFLOAT3& outMax);

    void ApplyUprightTorque(PBD::RigidBody& rb, float strength, float dt);

};