#pragma once

#include <directxmath.h>
#include <cstdint>

#include "Math/MathHelper.h"

namespace PBD
{

    struct PBDParticle;

    // ------------------------------------------------------------
    // 定数
    // ------------------------------------------------------------
    // 無効なハンドルを表すインデックス値
    inline constexpr int invalid_index = -1;

    // ------------------------------------------------------------
    // ボディ種類
    // ------------------------------------------------------------

    enum class body_type : std::uint8_t
    {
        particle,
        rigid_body,
        static_shape
    };

    // ------------------------------------------------------------
    // Body Handle
    //
    //  コンテナに格納されたボディを参照する軽量ハンドル
    // ------------------------------------------------------------

    struct body_handle
    {
        body_type type = body_type::particle;
        int index = invalid_index;

        // 有効なハンドルか判定
        constexpr bool is_valid() const noexcept
        {
            return index != invalid_index;
        }
        // 無効化する
        constexpr void invalidate() noexcept
        {
            index = invalid_index;
        }
    };

    // ------------------------------------------------------------
    // contact
    //
    // 衝突検出によって生成される純粋な幾何学接触情報。
    // Solver には依存しない。
    // ------------------------------------------------------------

    struct contact
    {
        float phi = 0.0f;                        // 符号付き距離
        DirectX::XMFLOAT3 normal{ 0,1,0 };         // 正規化済み法線
        DirectX::XMFLOAT3 position{ 0,0,0 };      // ワールド座標上の接触位置
        DirectX::XMFLOAT3 surfaceVelocity = { 0,0,0 };  // 表面速度

        float friction = 1.0f;// 摩擦係数

        int featureId = invalid_index;// 接触フィーチャID
    };

    // ------------------------------------------------------------
    // body_contact
    //
    // 2つのボディに関連付けられた接触情報
    // ------------------------------------------------------------

    struct body_contact
    {
        body_handle body_a;
        body_handle body_b;
        contact c;
    };

    // ------------------------------------------------------------
    // collision_shape
    //
    //  静的衝突形状の基底クラス
    // ------------------------------------------------------------

    class CollisionShape
    {
    public:

        explicit CollisionShape(std::uint32_t phase) noexcept
            : phase(phase)
        {
        }

        virtual ~CollisionShape() = default;

        virtual bool collide(
            const PBDParticle& p,
            contact& out_contact) const = 0;

        void BeginFrame()
        {
            accumulatedImpulse = { 0,0,0 };
            accumulatedTorque = { 0,0,0 };
        }

        virtual void AddImpulse(const DirectX::XMFLOAT3& normal,
            float correction,
            const DirectX::XMFLOAT3& contactPoint){}

        std::uint32_t phase = 0;

        DirectX::XMFLOAT3 accumulatedImpulse = {};; // 並進方向の衝撃
        DirectX::XMFLOAT3 accumulatedTorque = {};;  // 回転方向の衝撃
    };

    // ------------------------------------------------------------
    // plane_shape
    //
    // 無限平面
    //
    //      nｷx - d = 0
    // ------------------------------------------------------------

    class plane_shape final : public CollisionShape
    {
    public:
        plane_shape(
            DirectX::XMFLOAT3 n,
            float d,
            std::uint32_t phase)
            : CollisionShape(phase),
            normal(n),
            distance(d)
        {
            using namespace DirectX;
            XMStoreFloat3(&normal, XMVector3Normalize(XMLoadFloat3(&normal)));
        }

        bool collide(
            const PBDParticle& p,
            contact& out_contact) const override;

        DirectX::XMFLOAT3 normal{ 0,1,0 };
        float distance = 0.0f;
    };

    // ------------------------------------------------------------
    // 球形状
    //
    // |x - c| - R = 0
    // ------------------------------------------------------------

    class sphere_shape final : public CollisionShape
    {
    public:

        sphere_shape(
            DirectX::XMFLOAT3 center,
            float radius,
            std::uint32_t phase)
            : CollisionShape(phase),
            center(center),
            radius(radius)
        {
        }

        bool collide(
            const PBDParticle& p,
            contact& out_contact) const override;

        DirectX::XMFLOAT3 center{ 0,0,0 };
        float radius = 1.0f;

        DirectX::XMFLOAT3 angularVelocity = { 0.0f,0.0f,0.0f }; //　角速度
    };

    class box_shape final : public CollisionShape
    {
    public:
        box_shape(
            DirectX::XMFLOAT3 center,
            DirectX::XMFLOAT3 extent,
            std::uint32_t phase)
            : CollisionShape(phase), center(center), extent(extent)
        {
        }

        bool collide(const PBDParticle& p, contact& out_contact) const override;

        void AddImpulse(
            const DirectX::XMFLOAT3& normal,
            float correction,
            const DirectX::XMFLOAT3& contactPoint) override;

        DirectX::XMFLOAT3 center = { 0,0,0 };
        DirectX::XMFLOAT3 extent = { 1,1,1 };
        DirectX::XMFLOAT3X3 rotation; // 3x3 回転行列（ワールド）

    };
}
