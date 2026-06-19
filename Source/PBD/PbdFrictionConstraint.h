#pragma once

#include <vector>
#include <DirectXMath.h>
#include "PbdCollision.h"

namespace PBD
{
    struct ContactConstraint;
    struct PBDParticle;

    // ------------------------------------------------------------
    // friction_constraint
    //
    // PBD（Position Based Dynamics）において、
    // クーロン摩擦を近似するための
    // 接線方向（Tangential）の拘束。
    //
    // このConstraintは、
    // 法線方向のContact Constraint解決後に評価され、
    // 接触面上での滑り movement を制限する。
    //
    // ------------------------------------------------------------
    // 物理モデル:
    //
    //   |Δx_t| ≤ μ |Δx_n|
    //
    // ここで:
    //
    //   Δx_t
    //      = 接線方向の移動量（滑り）
    //
    //   Δx_n
    //      = 法線方向の位置補正量
    //        （めり込み解消量）
    //
    //   μ
    //      = 摩擦係数（friction coefficient）
    //
    // ------------------------------------------------------------
    // Notes:
    //
    // - これは Position Based な摩擦近似である
    //
    // - Velocity（速度）は明示的に保持しない
    //
    // - 代わりに位置差分から運動を推定する
    //
    // - normal_correction は関連する
    //   contact_constraint から計算され、
    //   摩擦量のスケーリングに使用される
    // ------------------------------------------------------------
    struct FrictionConstraint
    {
        // Constraintに関与する2つのBody
        body_handle a;
        body_handle b;

        // Contact Plane（接触面）上の接線方向
        // Contact Normalと直交する単位ベクトル。
        // 摩擦による滑り制限方向として使用される。
        DirectX::XMFLOAT3 tangent;

        // Contact Normal（接触法線）
        //
        // 単位ベクトル。
        //
        // 接触面の向きを定義する。
        DirectX::XMFLOAT3 normal;

        // クーロン摩擦係数 μ
        //
        // 値が大きいほど滑りにくくなる。
        //
        //   0.0 → 完全に滑る
        //   1.0 → 強い摩擦
        float friction = 1.0f;

        // 法線方向補正量 |Δx_n|
        //
        // Contact Constraintによる
        // 「押し戻し量」の大きさ。
        //
        // クーロン摩擦モデル:
        //
        //   |Δx_t| ≤ μ |Δx_n|
        //
        // に従って、
        // 接線方向移動量を制限するために使用する。
        float normal_correction = 0.0f;

        // 表面速度
        DirectX::XMFLOAT3 surfaceVelocity = { 0.0f,0.0f,0.0f };
    };


    // ------------------------------------------------------------
    // to_friction_constraint
    //
    // Collision Detection（衝突検出）の結果から、
    // 摩擦Constraintを生成する関数。
    //
    // ------------------------------------------------------------
    // Input:
    //
    //   body_contact
    //      → 衝突結果
    //
    //   contact_constraint
    //      → 法線方向Constraint
    //
    // ------------------------------------------------------------
    // Output:
    //
    //   Solver Projectionで使用可能な
    //   friction_constraint
    //
    // ------------------------------------------------------------
    // Responsibilities（役割）:
    //
    //   1. Body Handleをコピー
    //
    //   2. Contact Normalに直交する
    //      Tangent方向を計算
    //
    //   3. Contact Dataから
    //      摩擦係数を取得
    //
    //   4. Contact Constraintから
    //      法線補正量を導出
    //
    // ------------------------------------------------------------
    // Note:
    // Tangent方向は任意に選択されるが、
    // 必ずNormalと直交するようにする。
    //
    // これにより、
    // - Velocity依存
    // - フレーム依存
    // - 微小振動
    // による不安定性を避けられる。
    // ------------------------------------------------------------

    FrictionConstraint to_friction_constraint(
        const body_contact& bc,
        const ContactConstraint& cc);

}