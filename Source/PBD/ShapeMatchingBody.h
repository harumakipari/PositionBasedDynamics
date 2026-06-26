#pragma once
#include <vector>
#include <directxmath.h>
#include "IndexRange.h"

namespace PBD
{
    struct PBDParticle;

    // =========================================================
    // Shape Matching Bodies
    // =========================================================
    /* Shape Matching を用いたソフトボディ。
       各 Body は:
       - Particle 配列の一部を参照
       - Shape Matching によって形状を維持
       - Constraint Solver として動作
    
       NOTE:
       - Cluster 分割は未使用
       - 1 Body = 1 ShapeMatching Domain
     */
    struct ShapeMatchingBody
    {
        int instance_index = 0;

        // Range into world.particles[]
        IndexRange particle_range;

        // Rest offsets (q_i)
        std::vector<DirectX::XMFLOAT4> rest_offsets;

        // Rest center of mass (c0)
        DirectX::XMFLOAT4 rest_center_of_mass{};

        // Current COM (c)
        DirectX::XMFLOAT4 center_of_mass{};

        // Previous COM (for velocity estimation)
        DirectX::XMFLOAT4 previous_center_of_mass{};

        // Final transform (R or R*S)
        DirectX::XMMATRIX transform{ DirectX::XMMatrixIdentity() };

        // 剛体の回転
        DirectX::XMFLOAT4 rigid_rotation_quat;
        DirectX::XMFLOAT3 rigid_position;


        // Solver parameters
        float stiffness{ 1.0f };
        float deformation_blend{ 0.2f };

        // Precomputed inverse rest covariance
        DirectX::XMMATRIX Aqq_inv{ DirectX::XMMatrixIdentity() };

        bool active = true;
        float scale = 1.0f;
        bool constrain_rotation_to_y = false;

        // Core operations
        DirectX::XMVECTOR compute_center_of_mass(const std::vector<PBDParticle>& particles) const;

        void compute_rest_covariance(const std::vector<PBDParticle>& particles);

        void reset_to_rest_state(std::vector<PBDParticle>& particles);

        void set_position(std::vector<PBDParticle>& particles, const DirectX::XMFLOAT3& position);

        void translate(std::vector<PBDParticle>& particles, DirectX::FXMVECTOR delta);

        void rotate(std::vector<PBDParticle>& particles,
            const DirectX::XMVECTOR& rotation,
            const DirectX::XMVECTOR& center);

        // Main shape matching solve
        void project(std::vector<PBDParticle>& particles);
    };

};