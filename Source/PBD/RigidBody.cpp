#include "pch.h"
#include "RigidBody.h"

namespace PBD
{
    void IntegrateRigidBody(RigidBody& rb, float dt)
    {
        using namespace DirectX;

        // 重力を入れる
        //rb.linearVelocity.y -= 9.8f * dt;

        // 位置
        XMVECTOR p = XMLoadFloat3(&rb.position);
        XMVECTOR v = XMLoadFloat3(&rb.linearVelocity);
        p = XMVectorAdd(p, XMVectorScale(v, dt));
        XMStoreFloat3(&rb.position, p);

        // 回転（クォータニオン積分）
        XMVECTOR q = XMLoadFloat4(&rb.rotation);
        XMVECTOR w = XMLoadFloat3(&rb.angularVelocity); // (wx, wy, wz)

        // dq/dt = 0.5 * (ω * q)
        XMVECTOR dq = XMQuaternionMultiply(
            XMVectorSet(w.m128_f32[0], w.m128_f32[1], w.m128_f32[2], 0.0f),
            q);
        q = XMVectorAdd(q, XMVectorScale(dq, 0.5f * dt));
        q = XMQuaternionNormalize(q);
        XMStoreFloat4(&rb.rotation, q);

        // 簡単な減衰
        //float linDamp = 0.5f;
        //float angDamp = 0.9f;

        float linDamp = 2.0f;
        float angDamp = 0.5f;


        rb.linearVelocity.x *= (1.0f - linDamp * dt);
        rb.linearVelocity.y *= (1.0f - linDamp * dt);
        rb.linearVelocity.z *= (1.0f - linDamp * dt);

        rb.angularVelocity.x *= (1.0f - angDamp * dt);
        rb.angularVelocity.y *= (1.0f - angDamp * dt);
        rb.angularVelocity.z *= (1.0f - angDamp * dt);
    }

    void ApplyUprightTorque(PBD::RigidBody& rb, float strength, float dt)
    {
        using namespace DirectX;

        XMVECTOR q = XMLoadFloat4(&rb.rotation);

        // 現在のアップベクトル（ローカルの (0,1,0) を回転）
        XMVECTOR up = XMVector3Rotate(XMVectorSet(0, 1, 0, 0), q);
        XMVECTOR worldUp = XMVectorSet(0, 1, 0, 0);

        // up と worldUp のズレを回転軸として扱う
        XMVECTOR axis = XMVector3Cross(up, worldUp);
        float angle = XMVectorGetX(XMVector3Length(axis)); // 小さいほど傾きが少ない

        if (angle > 1e-3f)
        {
            axis = XMVectorScale(axis, 1.0f / angle); // 正規化

            // 傾きに比例したトルクを加える
            XMVECTOR w = XMLoadFloat3(&rb.angularVelocity);
            w = XMVectorAdd(w, XMVectorScale(axis, strength * angle * dt));
            XMStoreFloat3(&rb.angularVelocity, w);
        }
    }


    void SolvePlaneForRigidBody(PBD::RigidBody& rb, float planeY, float radius)
    {
        if (rb.position.y < planeY + radius)
        {
            rb.position.y = planeY + radius;

            if (rb.linearVelocity.y < 0.0f)
                rb.linearVelocity.y *= -0.3f; // 反発
        }
    }

    bool SolveBoxForRigidBody(PBD::RigidBody& rb,
        const DirectX::XMFLOAT3& boxMin,
        const DirectX::XMFLOAT3& boxMax,
        float radius)
    {
        using namespace DirectX;

        XMVECTOR p = XMLoadFloat3(&rb.position);
        XMVECTOR bMin = XMLoadFloat3(&boxMin);
        XMVECTOR bMax = XMLoadFloat3(&boxMax);

        XMVECTOR closest = XMVectorClamp(p, bMin, bMax);
        XMVECTOR v = XMVectorSubtract(p, closest);
        float dist = XMVectorGetX(XMVector3Length(v));

        float phi = dist - radius;
        if (phi >= 0.0f)
            return false;

        XMVECTOR n = (dist > 1e-6f)
            ? XMVectorScale(v, 1.0f / dist)
            : XMVectorSet(0, 1, 0, 0);

        // 押し出し
        p = XMVectorAdd(p, XMVectorScale(n, -phi));
        XMStoreFloat3(&rb.position, p);

        // 速度の法線成分を減衰
        XMVECTOR vel = XMLoadFloat3(&rb.linearVelocity);
        float vn = XMVectorGetX(XMVector3Dot(vel, n));
        if (vn < 0.0f)
        {
            vel = XMVectorSubtract(vel, XMVectorScale(n, vn * 1.2f));
            XMStoreFloat3(&rb.linearVelocity, vel);
        }

        return true;
    }

    void ComputeAABBFromOBB(
        const DirectX::XMFLOAT3& center,
        const DirectX::XMFLOAT3& extent,
        const DirectX::XMFLOAT3X3& rot,
        DirectX::XMFLOAT3& outMin,
        DirectX::XMFLOAT3& outMax)
    {
        using namespace DirectX;

        XMVECTOR c = XMLoadFloat3(&center);
        XMVECTOR e = XMLoadFloat3(&extent);

        // 回転行列
        XMMATRIX R = XMLoadFloat3x3(&rot);

        // 各軸の絶対値を取る（OBB → AABB の基本）
        XMMATRIX absR = {
            XMVectorAbs(R.r[0]),
            XMVectorAbs(R.r[1]),
            XMVectorAbs(R.r[2]),
            XMVectorSet(0,0,0,1)
        };

        // AABB の半径 = abs(R) * extent
        XMVECTOR half = XMVector3TransformNormal(e * 0.5f, absR);

        XMVECTOR bMin = c - half;
        XMVECTOR bMax = c + half;

        XMStoreFloat3(&outMin, bMin);
        XMStoreFloat3(&outMax, bMax);
    }


}