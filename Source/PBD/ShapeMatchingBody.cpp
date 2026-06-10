#include "pch.h"
#include "ShapeMatchingBody.h"
#include "PbdParticle.h"
#include <cmath>

using namespace DirectX;

namespace PBD
{
	/// 粒子群を元の形へ戻そうとする

	// ------------------------------------------------------------
	// NaNチェック（デバッグ安全用）
	// ------------------------------------------------------------
	inline bool contains_nan(const XMVECTOR& v)
	{
		return XMVector4IsNaN(v);
	}

	inline bool contains_nan(const XMMATRIX& m)
	{
		return XMVector4IsNaN(m.r[0]) ||
			XMVector4IsNaN(m.r[1]) ||
			XMVector4IsNaN(m.r[2]) ||
			XMVector4IsNaN(m.r[3]);
	}

	// ------------------------------------------------------------
	// Y軸回転制限
	//
	// Pitch/Roll を除去し、Yaw のみ保持する
	// ------------------------------------------------------------
	static XMMATRIX constrain_rotation_y(const XMMATRIX& R)
	{
		XMVECTOR forward = R.r[2];

		XMVECTOR forward_xz = XMVectorSet(
			XMVectorGetX(forward),
			0.0f,
			XMVectorGetZ(forward),
			0.0f);

		forward_xz = XMVector3Normalize(forward_xz);

		const XMVECTOR up = XMVectorSet(0, 1, 0, 0);
		const XMVECTOR right = XMVector3Cross(up, forward_xz);

		XMMATRIX out;
		out.r[0] = right;
		out.r[1] = up;
		out.r[2] = forward_xz;
		out.r[3] = XMVectorSet(0, 0, 0, 1);

		return out;
	}

	// ------------------------------------------------------------
	// 極分解
	//
	// 回転行列を反復的に抽出する
	// ------------------------------------------------------------
	static XMMATRIX polar_decomposition(const XMMATRIX& A, int iterations = 5)
	{
		XMMATRIX R = A;

		for (int i = 0; i < iterations; ++i)
		{
			XMMATRIX R_inv = XMMatrixInverse(nullptr, R);
			XMMATRIX R_invT = XMMatrixTranspose(R_inv);
			R = 0.5f * (R + R_invT);
		}

		float det = XMVectorGetX(XMMatrixDeterminant(R));

		if (fabs(det) < 1e-6f)
			return XMMatrixIdentity();

		// Orthonormalization (Gram-Schmidt)
		XMVECTOR X = XMVector3Normalize(R.r[0]);

		XMVECTOR Y = R.r[1];
		Y = XMVectorSubtract(Y, XMVectorScale(X, XMVectorGetX(XMVector3Dot(X, Y))));
		Y = XMVector3Normalize(Y);

		XMVECTOR Z = XMVector3Cross(X, Y);

		if (XMVectorGetX(XMVector3Dot(XMVector3Cross(X, Y), Z)) < 0.0f)
			Z = XMVectorNegate(Z);

		return XMMATRIX(X, Y, Z, XMVectorSet(0, 0, 0, 1));
	}

	// ------------------------------------------------------------
	// project
	//
	// Shape Matching のコア解法
	//
	//   1.  重心計算
	//   2.  A_pq 構築
	//   3.  A計算　Rest状態の共分散逆行列 
	//   4. 回転抽出 R
	//   5. 変形行列S
	//   6. 投影
	// ------------------------------------------------------------
	void ShapeMatchingBody::project(std::vector<PBDParticle>& particles)
	{
		const int begin = particle_range.offset;
		const int end = begin + particle_range.count;

		if (begin == end)
			return;

		//----------------------------------------------------------
		// 1. 重心計算
		//----------------------------------------------------------
		XMVECTOR c = compute_center_of_mass(particles);

		previous_center_of_mass = center_of_mass;
		XMStoreFloat4(&center_of_mass, c);

		//----------------------------------------------------------
		// 2. A_pq 構築
		//----------------------------------------------------------
		XMMATRIX Apq = XMMatrixSet(
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0);

		for (int i = begin, local = 0; i < end; ++i, ++local)
		{
			const PBDParticle& p = particles[i];

			if (p.inverse_mass == 0.0f)
				continue;

			const float m = 1.0f / p.inverse_mass;

			const XMVECTOR x = XMLoadFloat3(&p.position);
			XMVECTOR q = XMLoadFloat4(&rest_offsets[local]);
			q = XMVectorScale(q, scale);

			const XMVECTOR p_rel = (x - c) * m;

			Apq.r[0] = XMVectorMultiplyAdd(p_rel, XMVectorSplatX(q), Apq.r[0]);
			Apq.r[1] = XMVectorMultiplyAdd(p_rel, XMVectorSplatY(q), Apq.r[1]);
			Apq.r[2] = XMVectorMultiplyAdd(p_rel, XMVectorSplatZ(q), Apq.r[2]);
		}

		Apq.r[3] = XMVectorSet(0, 0, 0, 1);

		//----------------------------------------------------------
		// 3. A計算　Rest状態の共分散逆行列 
		//----------------------------------------------------------
		const float inv_s2 = 1.0f / (scale * scale);
		XMMATRIX A = XMMatrixMultiply(Aqq_inv * inv_s2, Apq);

		//----------------------------------------------------------
		// 4. 回転抽出 R
		//----------------------------------------------------------
		XMMATRIX R = polar_decomposition(A);

		if (constrain_rotation_to_y)
			R = constrain_rotation_y(R);

		if (XMVectorGetX(XMMatrixDeterminant(R)) < 0.0f)
			R.r[2] = XMVectorNegate(R.r[2]);

		//----------------------------------------------------------
		// 5. 変形行列S
		//----------------------------------------------------------
		if (deformation_blend > 0.0f)
		{
			XMMATRIX S = XMMatrixMultiply(XMMatrixTranspose(R), A);

			float det = XMVectorGetX(XMMatrixDeterminant(S));

			if (det > 1e-6f)
			{
				float inv_vol = 1.0f / std::cbrt(det);
				S = XMMatrixMultiply(XMMatrixScaling(inv_vol, inv_vol, inv_vol), S);

				S = 0.5f * (S + XMMatrixTranspose(S));

				XMMATRIX I = XMMatrixIdentity();

				for (int i = 0; i < 3; ++i)
					S.r[i] = XMVectorLerp(I.r[i], S.r[i], deformation_blend);

				S.r[3] = XMVectorSet(0, 0, 0, 1);

				R = XMMatrixMultiply(S, R);
			}
		}

		transform = R;

		//----------------------------------------------------------
		// 6. 投影
		//----------------------------------------------------------
		for (int i = begin, local = 0; i < end; ++i, ++local)
		{
			PBDParticle& p = particles[i];

			if (p.inverse_mass == 0.0f)
				continue;

			XMVECTOR q = XMLoadFloat4(&rest_offsets[local]);
			q = XMVectorScale(q, scale);

			XMVECTOR goal = c + XMVector3TransformNormal(q, R);

			XMVECTOR x = XMLoadFloat3(&p.position);
			x = XMVectorLerp(x, goal, stiffness);

			XMStoreFloat3(&p.position, x);
		}
	}

	// -----------------------------------------------------------------------------
	// 重心計算
	//
	//     c = Σ m_i x_i / Σ m_i
	//     逆質量が0の粒子は無視する。
	// -----------------------------------------------------------------------------
	XMVECTOR ShapeMatchingBody::compute_center_of_mass(const std::vector<PBDParticle>& particles) const
	{
		XMVECTOR c = XMVectorZero();
		float mass_sum = 0.0f;

		const int begin = particle_range.offset;
		const int end = begin + particle_range.count;

		for (int i = begin; i < end; ++i)
		{
			const PBDParticle& p = particles[i];

			if (p.inverse_mass == 0.0f)
				continue;

			const float m = 1.0f / p.inverse_mass;

			c += XMLoadFloat3(&p.position) * m;
			mass_sum += m;
		}

		if (mass_sum == 0.0f)
			return XMVectorZero();

		return c / mass_sum;
	}

	// -----------------------------------------------------------------------------
	// Rest共分散行列計算
	//
	//     Aqq = Σ m_i q_i q_i^T
	//
	// 実行時用に逆行列を事前計算
	// -----------------------------------------------------------------------------
	void ShapeMatchingBody::compute_rest_covariance(const std::vector<PBDParticle>& particles)
	{
		const int begin = particle_range.offset;
		const int end = begin + particle_range.count;

		XMMATRIX Aqq = XMMatrixSet(
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0,
			0, 0, 0, 0);

		for (int i = begin, local = 0; i < end; ++i, ++local)
		{
			const PBDParticle& p = particles[i];

			if (p.inverse_mass == 0.0f)
				continue;

			const float m = 1.0f / p.inverse_mass;

			XMVECTOR q = XMLoadFloat4(&rest_offsets[local]);
			q = XMVectorScale(q, m);

			Aqq.r[0] = XMVectorMultiplyAdd(q, XMVectorSplatX(q), Aqq.r[0]);
			Aqq.r[1] = XMVectorMultiplyAdd(q, XMVectorSplatY(q), Aqq.r[1]);
			Aqq.r[2] = XMVectorMultiplyAdd(q, XMVectorSplatZ(q), Aqq.r[2]);
		}

		Aqq.r[3] = XMVectorSet(0, 0, 0, 1);

		Aqq_inv = XMMatrixInverse(nullptr, Aqq);
	}

	// -----------------------------------------------------------------------------
	// 初期状態へリセット
	//
	//     x_i = c0 + scale * q_i
	//
	// velocityもリセット
	// -----------------------------------------------------------------------------
    void ShapeMatchingBody::reset_to_rest_state(std::vector<PBDParticle>& particles)
	{
		const XMVECTOR c0 = XMLoadFloat4(&rest_center_of_mass);

		const int begin = particle_range.offset;
		const int end = begin + particle_range.count;

		for (int i = begin, local = 0; i < end; ++i, ++local)
		{
			PBDParticle& p = particles[i];

			XMVECTOR q = XMLoadFloat4(&rest_offsets[local]);
			q = XMVectorScale(q, scale);

			XMVECTOR x = XMVectorAdd(c0, q);

			XMStoreFloat3(&p.position, x);
			p.velocity = { 0.0f, 0.0f, 0.0f };
		}
	}

	// -----------------------------------------------------------------------------
	// 重心を指定位置へ移動
	// -----------------------------------------------------------------------------
	void ShapeMatchingBody::set_position(std::vector<PBDParticle>& particles, const XMFLOAT3& position)
	{
		const XMVECTOR current_com = compute_center_of_mass(particles);
		const XMVECTOR target = XMLoadFloat3(&position);
		const XMVECTOR delta = XMVectorSubtract(target, current_com);

		const int begin = particle_range.offset;
		const int end = begin + particle_range.count;

		for (int i = begin; i < end; ++i)
		{
			PBDParticle& p = particles[i];

			XMVECTOR x = XMLoadFloat3(&p.position);
			x = XMVectorAdd(x, delta);

			XMStoreFloat3(&p.position, x);
			p.velocity = { 0.0f, 0.0f, 0.0f };
		}
	}

	// -----------------------------------------------------------------------------
	// 平行移動
	//
	//     x_i += Δ
	// -----------------------------------------------------------------------------
	void ShapeMatchingBody::translate(std::vector<PBDParticle>& particles, FXMVECTOR delta)
	{
		const int begin = particle_range.offset;
		const int end = begin + particle_range.count;

		for (int i = begin; i < end; ++i)
		{
			PBDParticle& p = particles[i];

			XMVECTOR x = XMLoadFloat3(&p.position);
			x = XMVectorAdd(x, delta);

			XMStoreFloat3(&p.position, x);
			p.velocity = { 0.0f, 0.0f, 0.0f };
		}
	}

	// -----------------------------------------------------------------------------
	// 回転
	//
	//     x_i' = center + R (x_i - center)
	// -----------------------------------------------------------------------------
	void ShapeMatchingBody::rotate(
		std::vector<PBDParticle>& particles,
		const XMVECTOR& rotation,
		const XMVECTOR& center)
	{
		const int begin = particle_range.offset;
		const int end = begin + particle_range.count;

		for (int i = begin; i < end; ++i)
		{
			PBDParticle& p = particles[i];

			const XMVECTOR x = XMLoadFloat3(&p.position);

			XMVECTOR local = XMVectorSubtract(x, center);
			local = XMVector3Rotate(local, rotation);

			XMVECTOR rotated = XMVectorAdd(center, local);

			XMStoreFloat3(&p.position, rotated);
			p.velocity = { 0.0f, 0.0f, 0.0f };
		}
	}
}