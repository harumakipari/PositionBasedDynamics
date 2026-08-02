#include "pch.h"
#include "PBDCollision.h"
#include "PBDParticle.h"


using namespace DirectX;

bool PBD::plane_shape::collide(
    const PBDParticle& p,
    contact& out_contact) const
{
    // --------------------------------------------------------
    // データ読み込み
    //
    // x : Particle位置
    // n : 平面法線（正規化）
    // --------------------------------------------------------
    const XMVECTOR x = XMLoadFloat3(&p.position);
    const XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&normal));

    // --------------------------------------------------------
    // 符号付き距離（Signed Distance）
    //
    // φ = dot(x,n) - d - r
    //
    // x      : Particle中心位置
    // n      : 平面法線
    // d      : 平面原点からの距離
    // r      : Particle半径
    //
    // 判定:
    //
    //   φ > 0
    //      → 平面の外側
    //   φ = 0
    //      → 接触
    //   φ < 0
    //      → めり込み
    // --------------------------------------------------------

    const float phi =
        XMVectorGetX(XMVector3Dot(x, n)) -
        distance -
        p.radius;

    if (phi >= 0.0f)
        return false;

    // --------------------------------------------------------
    // 接触法線
    //
    // Solverが押し戻し方向として利用する
    // --------------------------------------------------------

    XMStoreFloat3(&out_contact.normal, n);

    // --------------------------------------------------------
    // めり込み量
    //
    // φ < 0 の値がそのまま保存される
    // --------------------------------------------------------
    out_contact.phi = phi;
    out_contact.friction = 0.5f; // TODO

    // --------------------------------------------------------
    // 接触点
    //
    // Particle中心から半径分だけ
    // 法線方向へ戻した位置
    //
    // x_contact = x - n * r
    // --------------------------------------------------------
    const XMVECTOR contact_point =
        XMVectorSubtract(x, XMVectorScale(n, p.radius));

    XMStoreFloat3(&out_contact.position, contact_point);

    return true;
}

bool PBD::sphere_shape::collide(
    const PBDParticle& p,
    contact& out_contact) const
{
    // --------------------------------------------------------
    // データ読み込み
    //
    // x : Particle位置
    // c : Sphere中心
    // --------------------------------------------------------
    const XMVECTOR x = XMLoadFloat3(&p.position);
    const XMVECTOR c = XMLoadFloat3(&center);

    // --------------------------------------------------------
    // Sphere中心 → Particleへのベクトル
    //
    // v = x - c
    // --------------------------------------------------------
    const XMVECTOR v = XMVectorSubtract(x, c);
    // Particle中心とSphere中心の距離
    const float dist = XMVectorGetX(XMVector3Length(v));

    // --------------------------------------------------------
    // 符号付き距離（Signed Distance）
    //
    // φ = |x - c| - (R + r)
    //
    // R : Sphere半径
    // r : Particle半径
    //
    // 判定:
    //
    //   φ > 0
    //      → 離れている
    //
    //   φ = 0
    //      → 接触
    //
    //   φ < 0
    //      → めり込み
    // --------------------------------------------------------
    const float phi = dist - (radius + p.radius);
    // 衝突していない
    if (phi >= 0.0f)
        return false;

    // --------------------------------------------------------
    // 衝突法線
    //
    // n = normalize(x - c)
    //
    // Sphere中心からParticle方向を向く
    // 単位ベクトル
    // --------------------------------------------------------
    XMVECTOR n;

    if (dist > 1e-6f)
    {
        n = XMVectorScale(v, 1.0f / dist);
    }
    else
    {
        // 特殊ケース
        // ParticleがSphere中心と
        // 完全に重なった場合
        // 正規化できないので
        // 仮の上方向ベクトルを使用する
        n = XMVectorSet(0, 1, 0, 0);
    }

    XMStoreFloat3(&out_contact.normal, n);

    // --------------------------------------------------------
    // めり込み量
    // --------------------------------------------------------
    out_contact.phi = phi;

    // 摩擦係数
    out_contact.friction = 0.5f;

    // --------------------------------------------------------
    // 接触点
    //
    // Particle表面上の接触位置
    //
    // x_contact = x - n * r
    // --------------------------------------------------------
    const XMVECTOR contact_point =
        XMVectorSubtract(x, XMVectorScale(n, p.radius));

    XMStoreFloat3(&out_contact.position, contact_point);

    // --------------------------------------------------------
    // 表面速度
    //
    // Particle表面上の速度
    // r = contact.position - center
    // v = ω × r
    // --------------------------------------------------------
    XMVECTOR omega = XMLoadFloat3(&angularVelocity);

    XMVECTOR r = XMVectorSubtract(contact_point, c);

    XMVECTOR surfaceVelocity = XMVector3Cross(omega, r);

    XMStoreFloat3(&out_contact.surfaceVelocity, surfaceVelocity);

    return true;
}

bool PBD::box_shape::collide(const PBDParticle& p, contact& out_contact) const
{
    using namespace DirectX;

    // 粒子位置（ワールド）
    XMVECTOR xw = XMLoadFloat3(&p.position);

    // 回転行列
    XMMATRIX R = XMLoadFloat3x3(&rotation);

    // ローカル空間へ変換
    XMVECTOR c = XMLoadFloat3(&center);
    XMVECTOR xl = XMVector3TransformNormal(xw - c, XMMatrixTranspose(R));

    // ローカル AABB の min/max
    XMFLOAT3 half = { extent.x * 0.5f, extent.y * 0.5f, extent.z * 0.5f };
    XMVECTOR bMin = XMVectorSet(-half.x, -half.y, -half.z, 0);
    XMVECTOR bMax = XMVectorSet(half.x, half.y, half.z, 0);

    // 最近接点（ローカル）
    XMVECTOR closest = XMVectorClamp(xl, bMin, bMax);

    // 球中心 → 最近接点
    XMVECTOR v = xl - closest;
    float dist = XMVectorGetX(XMVector3Length(v));
    float phi = dist - p.radius;

    // 外側からの衝突
    if (phi < 0.0f && dist > 1e-6f)
    {
        XMVECTOR n_local = v / dist;

        // 法線をワールドへ
        XMVECTOR n_world = XMVector3TransformNormal(n_local, R);
        XMStoreFloat3(&out_contact.normal, n_world);

        out_contact.phi = phi;
        out_contact.friction = 0.5f;

        // 接触点（ワールド）
        XMVECTOR contact_local = xl - n_local * p.radius;
        XMVECTOR contact_world = c + XMVector3TransformNormal(contact_local, R);
        XMStoreFloat3(&out_contact.position, contact_world);

        return true;
    }

    // 内部判定
    XMFLOAT3 xl3;
    XMStoreFloat3(&xl3, xl);

    bool inside =
        (xl3.x > -half.x && xl3.x < half.x) &&
        (xl3.y > -half.y && xl3.y < half.y) &&
        (xl3.z > -half.z && xl3.z < half.z);

    if (!inside)
        return false;

    // 内部 → 一番近い面を探す
    float dx_min = xl3.x + half.x;
    float dx_max = half.x - xl3.x;
    float dy_min = xl3.y + half.y;
    float dy_max = half.y - xl3.y;
    float dz_min = xl3.z + half.z;
    float dz_max = half.z - xl3.z;

    float min_dist = dx_min;
    XMFLOAT3 n_local3 = { -1,0,0 };

    if (dx_max < min_dist) { min_dist = dx_max; n_local3 = { 1,0,0 }; }
    if (dy_min < min_dist) { min_dist = dy_min; n_local3 = { 0,-1,0 }; }
    if (dy_max < min_dist) { min_dist = dy_max; n_local3 = { 0, 1,0 }; }
    if (dz_min < min_dist) { min_dist = dz_min; n_local3 = { 0,0,-1 }; }
    if (dz_max < min_dist) { min_dist = dz_max; n_local3 = { 0,0, 1 }; }

    // 法線（ワールド）
    XMVECTOR n_local = XMLoadFloat3(&n_local3);
    XMVECTOR n_world = XMVector3TransformNormal(n_local, R);
    XMStoreFloat3(&out_contact.normal, n_world);

    // めり込み量
    out_contact.phi = -(min_dist + p.radius);
    out_contact.friction = 0.5f;

    // 接触点（ワールド）
    XMVECTOR contact_local = xl - n_local * p.radius;
    XMVECTOR contact_world = c + XMVector3TransformNormal(contact_local, R);
    XMStoreFloat3(&out_contact.position, contact_world);

    return true;
}



void PBD::box_shape::AddImpulse(const DirectX::XMFLOAT3& normal, float correction, const DirectX::XMFLOAT3& contactPoint)
{
    auto impulse = MathHelper::Multiply(normal, correction);
    accumulatedImpulse = MathHelper::Add(accumulatedImpulse, impulse);

    auto r = MathHelper::Subtract(contactPoint, center);
    accumulatedTorque = MathHelper::Add(accumulatedTorque, MathHelper::Cross(r, impulse));

    Logger::Log(("accumulatedImpulse x:") + std::to_string(accumulatedImpulse.x) + ("y :") + std::to_string(accumulatedImpulse.y) + ("z :") + std::to_string(accumulatedImpulse.z));
}
