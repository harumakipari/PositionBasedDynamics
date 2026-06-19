#include "pch.h"
#include "PbdSolver.h"


#include <algorithm>
#include "PbdWorld.h"
#include "PbdContactConstraint.h"
#include "PbdFrictionConstraint.h"
#include "PbdDistanceConstraint.h"


namespace PBD
{
	// ------------------------------------------------------------
	// integrate
	//
	// PBDの予測ステップ（Prediction Step）を実行する。
	//
	// 処理内容:
	//
	//   1. Velocity（速度）更新
	//      （Explicit Euler 積分）
	//
	//   2. Position（位置）予測
	//
	// 数式:
	//
	//   v_i^{t+1} = v_i^t + dt * a_ext
	//   x_i*      = x_i^t + dt * v_i^{t+1}
	//
	// Notes:
	//
	// - previous_position には
	//   速度再構築用に x_i^t を保存する
	//
	// - 固定Particle（inverse_mass == 0）は
	//   更新しない
	// ------------------------------------------------------------
	void Solver::integrate(PBDWorld& world, float dt)
	{
		// 前フレーム位置を保存し、Velocityを積分
		for (PBDParticle& p : world.particles)
		{
			p.previousPosition = p.position;

			if (p.inverse_mass == 0.0f)
				continue;

			// 外力加速度を適用（例: 重力）
			p.velocity.x += world.gravity.x * dt;
			p.velocity.y += world.gravity.y * dt;
			p.velocity.z += world.gravity.z * dt;
		}

		// Positionを予測
		for (PBDParticle& p : world.particles)
		{
			if (p.inverse_mass == 0.0f)
				continue;

			p.position.x += p.velocity.x * dt;
			p.position.y += p.velocity.y * dt;
			p.position.z += p.velocity.z * dt;
		}
	}

	// ------------------------------------------------------------
	// generate_contacts
	//
	// Collision Detection（衝突検出）の結果を
	// Solver Constraintへ変換する。
	//
	// パイプライン:
	//
	//   1. Particle と Shape の衝突検出
	//
	//   2. body_contact として保存
	//
	//   3. Solver Constraintへ変換
	//
	// 現在の実装:
	//
	//   - 総当たり
	//     （Particle × Shape）
	//
	//   - 空間分割構造なし
	//
	// 将来的改善:
	//
	//   - Spatial Hashing
	//   - BVH
	//   - Contact Manifold
	// ------------------------------------------------------------
	void Solver::generate_contacts(PBDWorld& world)
	{
		std::vector<body_contact> contacts;
		contacts.reserve(world.particles.size());

		const int particle_count = static_cast<int>(world.particles.size());
		const int shape_count = static_cast<int>(world.collision_shapes.size());

		// ----------------------------------------------------
		// Narrow-phase collision detection
		// ----------------------------------------------------
		for (int p_index = 0; p_index < particle_count; ++p_index)
		{
			auto& p = world.particles[p_index];

			for (int s_index = 0; s_index < shape_count; ++s_index)
			{
				auto& s = world.collision_shapes[s_index];

				// Phase filtering
				if ((p.phase & s->phase) == 0)
					continue;

				contact c;

				if (!s->collide(p, c))
					continue;

				body_contact bc;
				bc.body_a = { body_type::particle, p_index };
				bc.body_b = { body_type::static_shape, s_index };
				bc.c = c;

				contacts.push_back(bc);
			}
		}

		// Solver Constraint を構築
		collision_constraints.clear();
		friction_constraints.clear();

		collision_constraints.reserve(contacts.size());
		friction_constraints.reserve(contacts.size());

		for (const body_contact& bc : contacts)
		{
			// 法線方向Constraint（めり込み防止）
			ContactConstraint cc = to_contact_constraint(bc);
			collision_constraints.push_back(cc);

			// 摩擦Constraint（接線方向）
			FrictionConstraint fc = to_friction_constraint(bc, cc);
			friction_constraints.push_back(fc);
		}
	}

	// ------------------------------------------------------------
	// step
	//
	// PBDシミュレーションを1ステップ実行する。
	//
	// パイプライン:
	//
	//   1. Force Integration（外力積分）
	//
	//   2. Position Prediction（位置予測）
	//
	//   3. Constraint生成
	//
	//   4. Constraint Projection反復
	//
	//   5. Velocity再構築
	//
	// Constraint Solving:
	//
	//   - Gauss-Seidel反復
	//
	//   - In-place位置更新
	//
	//   - 後続Constraintは
	//     更新済みPositionを見る
	// ------------------------------------------------------------
	void Solver::step(PBDWorld& world, float dt)
	{
		using namespace DirectX;

		// 1–2. Prediction
		integrate(world, dt);

		// 3. Contact generation
		generate_contacts(world);

		// 4. Constraint projection loop
		for (int iteration = 0; iteration < solver_iterations; ++iteration)
		{
			// Distance constraints
			for (const DistanceConstraint& dc : world.distance_constraints)
			{
				project_distance_constraint(world, dc);
			}

			// Contact (法線方向)
			for (const ContactConstraint& cc : collision_constraints)
			{
				project_contact_constraint(world, cc);
			}

			// Friction (接線方向)
			for (const FrictionConstraint& fc : friction_constraints)
			{
				project_friction_constraint(world, fc);
			}

			// Shape matching
			for (ShapeMatchingBody& body : world.bodies)
			{
				if (body.active)
					body.project(world.particles);
			}
		}

		// 5. Velocity 再構築 + damping
		const float damping = 2.0f;
		const float damping_factor = std::exp(-damping * dt);

		for (PBDParticle& p : world.particles)
		{
			if (p.inverse_mass == 0.0f)
				continue;

			// v = (x_new - x_old) / dt
			p.velocity.x = (p.position.x - p.previousPosition.x) / dt;
			p.velocity.y = (p.position.y - p.previousPosition.y) / dt;
			p.velocity.z = (p.position.z - p.previousPosition.z) / dt;

			// 指数減衰
			p.velocity.x *= damping_factor;
			p.velocity.y *= damping_factor;
			p.velocity.z *= damping_factor;
		}
	}

	// ------------------------------------------------------------
	// project_friction_constraint
	//
	// Position Based Coulomb Friction を適用する。
	//
	// 処理手順:
	//
	//   1. displacement を計算
	//        v = x - x_prev
	//
	//   2. 法線成分を除去
	//        → 接線方向移動
	//
	//   3. Coulomb LimitでClamp
	//
	//   4. Tangent方向へ補正適用
	// ------------------------------------------------------------
	void Solver::project_friction_constraint(PBDWorld& world, const FrictionConstraint& fc)
	{
		using namespace DirectX;

		if (fc.a.type != body_type::particle)
			return;

		PBDParticle& p = world.particles[fc.a.index];

		if (p.inverse_mass == 0.0f)
			return;

		XMVECTOR x = XMLoadFloat3(&p.position);
		XMVECTOR x_prev = XMLoadFloat3(&p.previousPosition);

		XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&fc.normal));

		// displacement（位置差分）
		XMVECTOR particleVelocity = XMVectorSubtract(x, x_prev);

		XMVECTOR surfaceVelocity = DirectX::XMLoadFloat3(&fc.surfaceVelocity);

		// TODO:表面速度を無効にする場合は下記を使用する
		//XMVECTOR relativeVelocity = particleVelocity;
		XMVECTOR relativeVelocity = XMVectorSubtract(particleVelocity, surfaceVelocity);

		// 法線成分除去
		float vn = XMVectorGetX(XMVector3Dot(relativeVelocity, n));
		XMVECTOR vt = XMVectorSubtract(relativeVelocity, XMVectorScale(n, vn));

		float vt_len = XMVectorGetX(XMVector3Length(vt));
		if (vt_len < 1e-6f)
			return;

		XMVECTOR t = XMVectorScale(vt, 1.0f / vt_len);

		// Coulomb limit
		float max_friction =
			fc.friction *
			fc.normal_correction /
			static_cast<float>(solver_iterations);

		float corr = -vt_len;
		corr = std::clamp(corr, -max_friction, max_friction);

		XMVECTOR dx = XMVectorScale(t, corr);
		x = XMVectorAdd(x, dx);

		XMStoreFloat3(&p.position, x);
	}

	// ------------------------------------------------------------
	// project_contact_constraint
	//
	// 接触拘束（Contact Constraint）を解き、
	// めり込んだParticleを接触法線方向へ押し戻す。
	//
	// 補正式:
	//
	//   Δx = -φ n
	//
	// ここで
	//
	//   φ < 0
	//
	// はめり込みを表す。
	//
	// StiffnessはSolverの反復回数に依存しないように
	// スケーリングされる。
	// ------------------------------------------------------------
	void Solver::project_contact_constraint(PBDWorld& world, const ContactConstraint& cc)
	{
		using namespace DirectX;

		if (cc.a.type != body_type::particle)
			return;

		PBDParticle& p = world.particles[cc.a.index];

		if (p.inverse_mass == 0.0f)
			return;

		if (cc.phi >= 0.0f)
			return;

		float correction =
			-cc.phi *
			(cc.stiffness / solver_iterations);

		XMVECTOR n =
			XMVector3Normalize(XMLoadFloat3(&cc.normal));

		XMVECTOR x = XMLoadFloat3(&p.position);
		XMVECTOR dx = XMVectorScale(n, correction);

		x = XMVectorAdd(x, dx);

		XMStoreFloat3(&p.position, x);
	}


	// --------------------------------------------------------
	// project_distance_constraint
	//
	// 2つのParticle間の距離を一定に保つConstraint。
	//
	// Positionを直接補正することで、
	// Rest Length（初期距離）を維持する。
	//
	// Constraint:
	//
	//   C(x1, x2)　= |x1 - x2| - rest_length = 0
	//
	// つまり:　現在距離 = rest_lengthを満たすように補正する。
	// 補正はGauss-Seidel方式で適用される。
	//
	// --------------------------------------------------------
	// Notes:
	// - inverse mass による重み付けを使用
	// - 両方Staticなら早期終了
	// - 非常に短い距離での
	//   数値不安定性対策あり
	// - Stiffness は
	//   PBDスタイルの単純スケーリング
	// --------------------------------------------------------
	void Solver::project_distance_constraint(PBDWorld& world, const DistanceConstraint& c)
	{
		using namespace DirectX;

		PBDParticle& p1 = world.particles[c.particle_a];
		PBDParticle& p2 = world.particles[c.particle_b];

		const float w1 = p1.inverse_mass;
		const float w2 = p2.inverse_mass;

		// 両方Staticなら補正不要
		const float w_sum = w1 + w2;
		if (w_sum == 0.0f)
			return;

		XMVECTOR x1 = XMLoadFloat3(&p1.position);
		XMVECTOR x2 = XMLoadFloat3(&p2.position);

		// Particle間ベクトルを計算
		XMVECTOR delta = XMVectorSubtract(x1, x2);
		float length = XMVectorGetX(XMVector3Length(delta));

		// 長さが極小なら終了
		// 0除算や不安定な正規化を防ぐ
		if (length < 1e-6f)
			return;

		// Constraint値
		//
		//   C = 現在距離 - Rest Length
		//
		//   C > 0 → 伸びている
		//   C < 0 → 縮んでいる
		float C = length - c.rest_length;

		// p2 → p1 方向の単位ベクトル
		XMVECTOR n = XMVectorScale(delta, 1.0f / length);

		// 補正量スカラー
		//
		// inverse mass に応じて分配する
		float s = -c.stiffness * C / w_sum;

		XMVECTOR correction = XMVectorScale(n, s);

		// inverse mass 重み付き補正
		if (w1 > 0.0f)
		{
			XMVECTOR x = XMVectorAdd(x1, XMVectorScale(correction, w1));
			XMStoreFloat3(&p1.position, x);
		}

		if (w2 > 0.0f)
		{
			XMVECTOR x = XMVectorSubtract(x2, XMVectorScale(correction, w2));
			XMStoreFloat3(&p2.position, x);
		}
	}


}