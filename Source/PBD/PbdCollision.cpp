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

    // 粒子中心
    XMVECTOR x = XMLoadFloat3(&p.position);

    // Box の AABB（ワールド）
    XMFLOAT3 halfExtent = {
        extent.x * 0.5f,
        extent.y * 0.5f,
        extent.z * 0.5f
    };

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

    // まずは通常の「外側から」の最近接点処理
    XMVECTOR closest = XMVectorClamp(x, bMin, bMax);

    XMVECTOR v = XMVectorSubtract(x, closest);
    float dist = XMVectorGetX(XMVector3Length(v));

    float phi = dist - p.radius;

    // ここで「外側から」の衝突を処理
    if (phi < 0.0f && dist > 1e-6f)
    {
        XMVECTOR n = XMVectorScale(v, 1.0f / dist);

        XMStoreFloat3(&out_contact.normal, n);
        out_contact.phi = phi;
        out_contact.friction = 0.5f;

        XMVECTOR contact_point = XMVectorSubtract(x, XMVectorScale(n, p.radius));
        XMStoreFloat3(&out_contact.position, contact_point);

        return true;
    }

    // ここから「完全に内部にいる場合」の処理
    // x が AABB の内部かどうか判定
    XMFLOAT3 px = p.position;

    bool inside =
        (px.x > worldMin.x && px.x < worldMax.x) &&
        (px.y > worldMin.y && px.y < worldMax.y) &&
        (px.z > worldMin.z && px.z < worldMax.z);

    if (!inside)
    {
        // 外側にいて、かつ最近接点でも当たっていない → 衝突なし
        return false;
    }

    // 内部にいるので、一番近い面を探す
    float dx_min = px.x - worldMin.x;
    float dx_max = worldMax.x - px.x;
    float dy_min = px.y - worldMin.y;
    float dy_max = worldMax.y - px.y;
    float dz_min = px.z - worldMin.z;
    float dz_max = worldMax.z - px.z;

    float min_dist = dx_min;
    XMFLOAT3 n = { -1.0f, 0.0f, 0.0f }; // 左面

    if (dx_max < min_dist) { min_dist = dx_max; n = { 1.0f, 0.0f, 0.0f }; }
    if (dy_min < min_dist) { min_dist = dy_min; n = { 0.0f,-1.0f, 0.0f }; }
    if (dy_max < min_dist) { min_dist = dy_max; n = { 0.0f, 1.0f, 0.0f }; }
    if (dz_min < min_dist) { min_dist = dz_min; n = { 0.0f, 0.0f,-1.0f }; }
    if (dz_max < min_dist) { min_dist = dz_max; n = { 0.0f, 0.0f, 1.0f }; }

    // 法線
    out_contact.normal = n;

    // めり込み量（負の値）
    // 「粒子中心が面から min_dist だけ内側にいる」＋「半径ぶん」
    out_contact.phi = -(min_dist + p.radius);

    out_contact.friction = 0.5f;

    // 接触点 = 粒子中心 - 法線 * 半径
    XMVECTOR n_vec = XMLoadFloat3(&n);
    XMVECTOR contact_point =
        XMVectorSubtract(x, XMVectorScale(n_vec, p.radius));

    XMStoreFloat3(&out_contact.position, contact_point);

    return true;
}
