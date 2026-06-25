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

    XMVECTOR x = XMLoadFloat3(&p.position);

    // 最近接点
    DirectX::XMFLOAT3 halfExtent = MathHelper::Multiply(extent, 0.5f);
    XMFLOAT3 worldMin = {
    center.x - halfExtent.x,
    center.y - halfExtent.y,
    center.z - halfExtent.z
    };

    XMFLOAT3 worldMax = {
        center.x + halfExtent.x,
        center.y + halfExtent.y,
        center.z + halfExtent.z
    };


    XMVECTOR bMin = XMLoadFloat3(&worldMin);
    XMVECTOR bMax = XMLoadFloat3(&worldMax);

    XMVECTOR closet = XMVectorClamp(x, bMin, bMax);

    // 球中心⇒最近接点
    XMVECTOR v = XMVectorSubtract(x, closet);
    float dist = XMVectorGetX(XMVector3Length(v));

    float phi = dist - p.radius;

    // 衝突していない
    if (phi >= 0.0f)
        return false;

    // 法線
    XMVECTOR n;
    if (dist > 1e-6f)
    {
        n = XMVectorScale(v, 1.0f / dist);
    }
    else
    {
        n = XMVectorSet(0, 1, 0, 0);
    }

    XMStoreFloat3(&out_contact.normal, n);

    // めり込み量
    out_contact.phi = phi;
    out_contact.friction = 0.5f;

    // 接触点
    XMVECTOR contact_point = XMVectorSubtract(x, XMVectorScale(n, p.radius));

    XMStoreFloat3(&out_contact.position, contact_point);

    return true;
}
