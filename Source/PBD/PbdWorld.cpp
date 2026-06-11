#include "pch.h"

#include "PbdWorld.h"
#include <numeric>
#include <DirectXCollision.h>
#include <random>

#include "PositionBasedDynamics.h"
#include "VoxelVolume.h"

namespace PBD
{
	/**
	 * -----------------------------------------------------------------------------
	 * spawn_shape_matching_body
	 *
	 * Shape Matching(PBD)を用いた変形可能オブジェクトを生成する。
	 *
	 * この関数では三角形メッシュからParticle群を生成し、
	 * shape_matching_body を初期化する。
	 *
	 * 生成されるParticleは以下の2種類。
	 *
	 *   ・表面Particle
	 *       メッシュ頂点から生成
	 *   ・内部Particle
	 *       Voxelizationによって生成
	 *
	 * -------------------------------------------------------------------------
	 * 内部Particleの重要性
	 *
	 * 内部Particleが無い場合
	 *
	 *   → 中身が空洞の殻として振る舞う
	 *   → 潰れやすい
	 *   → ボリューム保持が難しい
	 *
	 * 内部Particleがある場合
	 *
	 *   → 体積保持が自然に発生する
	 *   → 安定した変形になる
	 *   → Shape Matching が非常に強くなる
	 *
	 * -------------------------------------------------------------------------
	 * 処理の流れ
	 *
	 *   (1) 頂点をワールド座標へ変換
	 *   (2) AABB計算
	 *   (3) Voxelization
	 *   (4) Particle生成
	 *        ・表面Particle
	 *        ・内部Particle
	 *   (5) 質量分配
	 *   (6) Rest Center Of Mass計算
	 *   (7) Rest Offset(q_i)計算
	 *   (8) Rest Covariance(Aqq)計算
	 *   (9) ShapeMatchingBody初期化
	 *   (10) Worldへ登録
	 *
	 * -------------------------------------------------------------------------
	 * Shape Matching理論
	 *
	 * 目的：
	 *
	 *   Rest Shapeに最も近い変換Aを求める
	 *
	 *       Σ_i m_i || A * q_i - p_i ||^2
	 *
	 * を最小化する。
	 *
	 * ここで
	 *   q_i
	 *      Rest Poseでの重心からの相対位置
	 *   p_i
	 *      現在Poseでの重心からの相対位置
	 *
	 * Goal Position:
	 *
	 *   g_i = A q_i + c
	 *
	 * Particle補正:
	 *
	 *   x_i += stiffness (g_i - x_i)
	 *
	 * -----------------------------------------------------------------------------
	 */
	int PBDWorld::spawn_shape_matching_body(
		deformable_model* model,
		float stiffness,
		float deformation_blend,
		float particle_radius,
		float total_mass,
		int voxel_resolution)
	{
		using namespace DirectX;

		// -------------------------------------------------------------------------
		// STEP 1
		//
		// 頂点をワールド座標へ変換し、
		// Voxelization用のAABBを計算する。
		// -------------------------------------------------------------------------

		std::vector<XMFLOAT3> transformed_positions(model->positions);

		XMVECTOR min_v = g_XMFltMax;
		XMVECTOR max_v = g_XMFltMin;

		for (auto& position : transformed_positions)
		{
			XMVECTOR p = XMLoadFloat3(&position);

			min_v = XMVectorMin(min_v, p);
			max_v = XMVectorMax(max_v, p);
		}

		XMFLOAT3 min_value, max_value;
		XMStoreFloat3(&min_value, min_v);
		XMStoreFloat3(&max_value, max_v);

		// -------------------------------------------------------------------------
		// STEP 2
		//
		// MeshをVoxel化する。
		//
		// 各Voxelを
		//   inside
		//   outside
		// に分類する。
		//
		// inside voxelのみが
		// 内部Particle生成に使用される。
		// -------------------------------------------------------------------------

		voxel_volume voxels;

		if (voxel_resolution > 0)
		{
			float voxel_size = compute_voxel_size_from_resolution(
				min_value, max_value, voxel_resolution);

			voxels = voxelize_mesh(
				transformed_positions,
				model->indices,
				min_value,
				max_value,
				voxel_size);
		}

		int inside_count = voxels.count(voxel_state::inside);

		// -------------------------------------------------------------------------
		// STEP 3
		//
		// Particle数を決定する。
		//
		//   表面Particle数
		// + 内部Particle数
		//
		// がBody全体のParticle数となる。
		// -------------------------------------------------------------------------

		const int begin = static_cast<int>(particles.size());

		int surface_particle_count = static_cast<int>(transformed_positions.size());
		int interior_particle_count = inside_count;

		int count = surface_particle_count + interior_particle_count;

		// -------------------------------------------------------------------------
		// STEP 4
		//
		// Particleへ質量を均等分配する。
		//
		//   m_i = total_mass / N
		//
		// Solverでは逆質量を使用するため
		//
		//   inverse_mass = 1 / m_i
		//
		// を保存する。
		// -------------------------------------------------------------------------

		float inverse_mass = 1.0f / (total_mass / count);

		particles.reserve(begin + count);

		// Helper for particle construction
		auto create_particle = [&](const XMFLOAT3& position, float inv_mass, float radius, int phase)
			{
				PBDParticle p{};

				p.position = position;
				p.previousPosition = position;
				p.velocity = { 0,0,0 };

				p.inverse_mass = inv_mass;
				p.radius = radius;
				p.phase = phase;

				particles.emplace_back(std::move(p));
			};

		// -------------------------------------------------------------------------
		// STEP 5
		//
		// Mesh頂点から表面Particleを生成する。
		// -------------------------------------------------------------------------

		for (auto position : transformed_positions)
		{
			create_particle(position, inverse_mass, particle_radius, 1);
		}

		// -------------------------------------------------------------------------
		// STEP 6
		//
		// Inside Voxelから内部Particleを生成する。
		//
		// Voxel中心にParticleを配置する。
		//
		// Grid状アーティファクトや
		// 共分散行列の特異化を防ぐため
		// ランダムなJitterを加える。
		// -------------------------------------------------------------------------

		if (interior_particle_count > 0)
		{
			std::mt19937 rng(std::random_device{}());
			std::uniform_real_distribution<float> jitter_dist(-0.5f, 0.5f);

			float jitter_amount = voxels.cell_size * 0.25f;

			for (int x = 0; x < voxels.nx; x++)
				for (int y = 0; y < voxels.ny; y++)
					for (int z = 0; z < voxels.nz; z++)
					{
						const auto& voxel = voxels.at(x, y, z);

						if (voxel.state == voxel_state::inside)
						{
							XMFLOAT3 position = voxels.center(x, y, z);

							position.x += jitter_dist(rng) * jitter_amount;
							position.y += jitter_dist(rng) * jitter_amount;
							position.z += jitter_dist(rng) * jitter_amount;

							create_particle(position, inverse_mass, particle_radius, 1);
						}
					}
		}

		// -------------------------------------------------------------------------
		// STEP 7
		//
		// Rest Poseの重心(c0)を計算する。
		//
		//   c0 = Σ(m_i x_i) / Σ(m_i)
		//
		// Shape Matchingの基準座標系になる。
		// -------------------------------------------------------------------------

		XMVECTOR c = XMVectorZero();
		float mass_sum = 0.0f;

		for (int i = 0; i < count; i++)
		{
			const PBDParticle& p = particles[begin + i];

			if (p.inverse_mass == 0.0f)
				continue;

			float mass = 1.0f / p.inverse_mass;

			mass_sum += mass;
			c += XMLoadFloat3(&p.position) * mass;
		}

		if (mass_sum > 0)
			c /= mass_sum;

		// -------------------------------------------------------------------------
		// STEP 8
		//
		// ShapeMatchingBodyを初期化する。
		// -------------------------------------------------------------------------

		ShapeMatchingBody body;

		body.particle_range = { begin, count };
		body.stiffness = stiffness;
		body.deformation_blend = deformation_blend;

		XMStoreFloat4(&body.center_of_mass, c);
		XMStoreFloat4(&body.rest_center_of_mass, c);

		// -------------------------------------------------------------------------
		// STEP 9
		//
		// Rest Offset(q_i)を計算する。
		//
		//   q_i = x_i - c0
		//
		// Rest Poseにおける
		// 重心からの相対位置を保存する。
		// -------------------------------------------------------------------------

		body.rest_offsets.resize(count);

		for (int i = 0; i < count; i++)
		{
			XMVECTOR x = XMLoadFloat3(&particles[begin + i].position);
			XMVECTOR qi = x - c;

			XMStoreFloat4(&body.rest_offsets[i], qi);
		}

		body.transform = XMMatrixIdentity();

		// -------------------------------------------------------------------------
		// STEP 10
		//
		// Rest Covariance Matrix(Aqq)を計算する。
		//
		//   Aqq = Σ m_i q_i q_i^T
		//
		// Shape Matchingで
		//
		//   A = Apq Aqq^-1
		//
		// を求めるために使用する。
		// -------------------------------------------------------------------------

		body.compute_rest_covariance(particles);

		// -------------------------------------------------------------------------
		// STEP 11
		//
		// shape_matching_bodyをWorldへ登録する。
		//
		// 登録後はSolverによって
		// 毎フレーム更新される。
		// -------------------------------------------------------------------------
	    const int index = static_cast<int>(bodies.size());
		bodies.emplace_back(std::move(body));

		return index;
	}
}

