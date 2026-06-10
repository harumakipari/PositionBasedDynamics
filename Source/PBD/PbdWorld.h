#pragma once

#include <vector>
#include <memory>

#include "DeformableModel.h"
#include "PbdCollision.h"
#include "PbdDistanceConstraint.h"
#include "PbdParticle.h"
#include "ShapeMatchingBody.h"
#include "PbdParticle.h"

namespace PBD
{
    /* PBD シミュレーション全体を管理するワールド。
       PBD に必要な全データを保持する中央コンテナ。
       この構造は Data Oriented Design を意識しており、
       各シミュレーション要素を連続メモリ上に保持する。
       ソルバはこの World を直接操作してシミュレーションを行う。
     */

    struct PBDWorld
    {
        // =========================================================
        // Particles
        // =========================================================
        /* シミュレーションで使用する全 Particle 配列。
           PBD の各処理はインデックス経由で
           この配列へ直接アクセスする。
           Particle は PBD の最小シミュレーション単位。
           保持する情報:
           - position
           - velocity
           - inverseMass
           - collision radius
           NOTE:
           - 各 Particle は連続メモリ上に格納される
           - 間接参照バッファを持たない
           - 基本的に1つの Body に所属する
         */
        std::vector<PBDParticle> particles;

		// =========================================================
		// Shape Matching Bodies
		// =========================================================

		/*
			Shape Matching によってシミュレーションされる変形物体。
		
			各ボディは：
				- 連続した Particle 範囲を参照する
				- 単一のグローバル Shape Matching 解法を行う
				- コントローラ兼ソルバとして動作する
		
			NOTE:
				- クラスタシステムは使用しない
				- 1ボディ = 1 Shape Matching 領域
		*/
		std::vector<ShapeMatchingBody> bodies;

		// =========================================================
		// Constraints
		// =========================================================

		/*
			距離制約。
		
			2つの Particle 間の距離を一定に保つ。
		
			用途:
				- Cloth シミュレーション
				- Rope / Chain システム
				- SoftBody の構造補強
		
			NOTE:
				Shape Matching と組み合わせることで剛性を高められる。
		*/
        std::vector<DistanceConstraint> distance_constraints;

		/*
			衝突形状。

			unique_ptr によるポリモーフィック管理。

			用途:
				- 静的環境（Plane / Box / Sphere）
				- 動的衝突
		*/
        std::vector<std::unique_ptr<CollisionShape>> collision_shapes;

		/*
			全 Particle に加える重力。 (e.g. gravity).
		*/
		DirectX::XMFLOAT3 gravity = { 0.0f, -9.8f, 0.0f }; // m/s?

		// =========================================================
		// Functions
		// =========================================================

		/*
			新しい Particle を生成して World に登録する。
		
			初期化する内容:
				- position / previous_position
				- velocity
				- inverse mass
				- collision radius
				- phase（フィルタリング用）
		
			戻り値:
				生成された Particle のインデックス
		*/
        int spawn_particle(
			DirectX::XMFLOAT3 position,
			DirectX::XMFLOAT3 velocity,
			float inverse_mass,
			float radius,
			std::uint32_t phase)
		{
			const int index = static_cast<int>(particles.size());

			auto& particle = particles.emplace_back();
			particle.position = particle.previousPosition = position;
			particle.velocity = velocity;
			particle.inverse_mass = inverse_mass;
			particle.radius = radius;
			particle.phase = phase;

			return index;
		}

		PBDParticle& get_particle(int index)
		{
			return particles[index];
		}

		const PBDParticle& get_particle(int index) const
		{
			return particles[index];
		}

		/*
			型 T の Collision Shape を生成して登録する。

			条件:
				- T は collision_shape を継承している必要がある

			Notes:
				- Shape の所有権は World が持つ
				- ライフタイムは内部管理
				- 返された index でアクセス可能

			Returns:
				作成された Shape の index
		*/
        template<typename T, typename... A>
		int spawn_collision_shape(A&&... args)
		{
			static_assert(std::is_base_of_v<CollisionShape, T>,
				"Shape must derive from collision_shape");

			const int index = static_cast<int>(collision_shapes.size());

			collision_shapes.emplace_back(
				std::make_unique<T>(std::forward<A>(args)...)
			);

			return index;
		}

		CollisionShape& get_collision_shape(int index)
		{
			return *collision_shapes[index];
		}

		const CollisionShape& get_collision_shape(int index) const
		{
			return *collision_shapes[index];
		}

		template<typename T>
		T& get_collision_shape_as(int index)
		{
			return static_cast<T&>(*collision_shapes[index]);
		}

		/*
			deformable_model から Shape Matching Body を生成・登録する。

			この関数は：
				- Particle を生成する（表面 + 内部）
				- Rest Configuration を初期化する
				- 共分散データを計算する
				- Body を World に登録する

			結果として：
				- Body は Particle 群を内部 index で参照する
				- 単一の Shape Matching 解法を行う

			Notes:
				- クラスタリングは使用しない
				- Body の所有権は World が持つ
				- ライフタイムは内部管理
				- 返された index を使って、
                  world.shape_matching_bodies 経由で
                  その Body にアクセスできます

			Returns:
				作成された Shape Matching Body の index
		*/
        int spawn_shape_matching_body(
			deformable_model* model,
			float stiffness,
			float deformation_blend,
			float particle_radius,
			float total_mass,
			int voxel_resolution);

		ShapeMatchingBody& get_shape_matching_body(int index)
		{
			return bodies[index];
		}

		const ShapeMatchingBody& get_shape_matching_body(int index) const
		{
			return bodies[index];
		}

		/*
			2 Particle 間に Distance Constraint を生成する。

			この制約は：
				- 2粒子間の距離を維持する
				- Constraint Projection で解決される

			Parameters:
				particle_a     - 1つ目の粒子
				particle_b     - 2つ目の粒子
				rest_length    - 保持したい距離
				stiffness      - 剛性 (0..1)

			Notes:
				- Constraint の所有権は World が持つ
				- ライフタイムは内部管理
				- 返された index を使って、
				  world.distance_constraints 経由で
                  その Constraint にアクセスできます

			Returns:
				作成された Constraint の index
		*/
        int spawn_distance_constraint(
			int particle_a,
			int particle_b,
			float rest_length,
			float stiffness = 1.0f)
		{
			DistanceConstraint c;
			c.particle_a = particle_a;
			c.particle_b = particle_b;
			c.rest_length = rest_length;
			c.stiffness = stiffness;

			const int index = static_cast<int>(distance_constraints.size());
			distance_constraints.emplace_back(std::move(c));

			return index;
		}

    };

};