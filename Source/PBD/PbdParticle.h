#pragma once
#include <directxmath.h>
#include <cstdint>

#include "VoxelVolume.h"

enum class voxel_state;

namespace PBD
{
    // ------------------------------------------------------------
    // PBDParticle
    //
    // Position Based Dynamics(PBD) で使用する粒子。
    // ソフトボディ・クロス・流体などの最小シミュレーション単位。
    //
    // PBDでは「速度」よりも「位置」を直接修正する。
    // Constraint によって粒子位置を補正し、挙動を安定させる。
    // ------------------------------------------------------------
    struct alignas(16) PBDParticle
    {
        // --------------------------------------------------------
        // Transform / Movement
        // --------------------------------------------------------

        // 現在位置 (x_i)
        DirectX::XMFLOAT3 position{};

        // 前フレーム位置
        //
        // PBDでは位置更新後に
        // velocity = (current - previous) / dt
        // で速度を再構築するため保持している。
        DirectX::XMFLOAT3 previousPosition{};

        // 速度
        //
        // PBDでは補助的な値。
        // 実際の挙動は Position Constraint によって決まる。
        DirectX::XMFLOAT3 velocity{};


        // --------------------------------------------------------
        // Physical properties
        // --------------------------------------------------------

        // 逆質量 (1 / mass)
        //
        // 0 の場合は固定粒子として扱う。
        // (無限質量 = 動かない)
        float inverse_mass{ 1.0f };

        // 粒子の当たり判定半径
        //
        // PBDでは粒子を「点」ではなく
        // 小さい球として扱う。
        float radius{ 0.0f };


        // --------------------------------------------------------
        // Collision filtering
        // --------------------------------------------------------

         // 衝突レイヤー / グループマスク
        std::uint32_t phase{ 0 };

        // voxel_state
        voxel_state voxelState = voxel_state::unknown;
    };

};