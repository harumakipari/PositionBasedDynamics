#pragma once

#include <stdint.h>

namespace PBD
{

    /*
        Position Based Dynamics で使用される距離制約。

        2つの粒子間の距離を一定に維持する。
    */
    struct DistanceConstraint
    {
        // 制約対象となる粒子のインデックス
        int particle_a;
        int particle_b;

        // 制約の基準距離
        float rest_length;

        // 制約の硬さ (0..1)
        float stiffness = 1.0f;
    };

}