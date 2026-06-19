#include "pch.h"
#include "PbdFrictionConstraint.h"

#include <algorithm>

#include "PbdParticle.h"
#include "PbdContactConstraint.h"
#include "PbdCollision.h"

using namespace DirectX;

namespace PBD
{
    // ------------------------------------------------------------
    // to_friction_constraint
    //
    // Collision Detection（衝突検出）の結果を、
    // Solverで使用可能な friction_constraint に変換する。
    //
    // 処理手順:
    //
    //   1. contact constraint から Body Handle をコピー
    //
    //   2. Contact Normal を正規化
    //
    //   3. Normal に直交する Tangent を生成
    //
    //   4. Collision Data から摩擦係数を取得
    //
    //   5. 法線方向補正量を計算
    //
    // ------------------------------------------------------------
    // Notes:
    //
    // - Tangent は Velocity（速度）からは生成しない。
    //
    //   代わりに、
    //   安定した直交ベクトルを使用する:
    //
    //        t ⟂ n
    //
    //   （tangent は normal に直交）
    //
    //   これにより:
    //
    //   - 数値安定性向上
    //   - 微小振動（jitter）軽減
    //   - フレーム依存性低減
    //
    //   が得られる。
    //
    // ------------------------------------------------------------
    // - normal_correction は
    //   めり込み深度から導出される:
    //
    //        Δx_n ≈ -φ * stiffness   (φ < 0 の場合)
    //
    //   ここで:
    //
    //      φ
    //         = 符号付き距離
    //
    //      stiffness
    //         = Constraint の硬さ
    //
    //   保存するのは
    //   正の大きさのみ:
    //
    //        |Δx_n|
    //
    // ------------------------------------------------------------
    FrictionConstraint to_friction_constraint(
        const body_contact& bc,
        const ContactConstraint& cc)
    {
        FrictionConstraint fc;

        // Constraintに関与するBodyをコピー
        fc.a = cc.a;
        fc.b = cc.b;

        const contact& c = bc.c;

        // Contact Normal を読み込み正規化
        XMVECTOR n = XMLoadFloat3(&cc.normal);
        n = XMVector3Normalize(n);

        // Tangent方向を生成
        //
        // Normal に直交するベクトルを構築する:
        //
        //     t ⟂ n
        //
        // これは接触面上の方向を定義する。
        //
        // XMVector3Orthogonal を使用することで、
        // Velocity依存推定を行わずに、
        // 安定した Tangent を生成できる。
        XMVECTOR t = XMVector3Orthogonal(n);
        t = XMVector3Normalize(t);

        XMStoreFloat3(&fc.tangent, t);
        XMStoreFloat3(&fc.normal, n);

        // 摩擦係数
        fc.friction = c.friction;

        // 法線方向補正量
        //
        // Contact Constraintから導出:
        //
        //   φ < 0 → めり込み
        //
        //   Δx_n ≈ -φ * stiffness
        //
        // 正の大きさのみ保存:
        //
        //   |Δx_n| = max(0, -φ * stiffness)
        //
        // これは摩擦制限:
        //
        //   |Δx_t| ≤ μ |Δx_n|
        //
        // の基準として使用される。
        fc.normal_correction =
            std::max<float>(0.0f, -cc.phi * cc.stiffness);

        fc.surfaceVelocity = c.surfaceVelocity;

        return fc;
    }

}