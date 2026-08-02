#include "pch.h"
#include "PbdContactConstraint.h"
#include "PbdParticle.h"

namespace PBD
{
	// ------------------------------------------------------------
	// to_contact_constraint
	//
	// Collision Detection（衝突検出）の結果から、
	// Solver用の contact_constraint を生成する関数。
	//
	// この関数は非常に軽量な変換関数であり、
	//
	//   衝突検出結果
	//        ↓
	//   Solver用Constraint
	//
	// への変換だけを担当する。
	//
	// 追加の物理計算は行わず、
	// データをコピーして整理するのみ。
	// ------------------------------------------------------------
    ContactConstraint to_contact_constraint(const body_contact& bc, float stiffness)
	{
		ContactConstraint cc;

		// --------------------------------------------------------
		// Body Handle をコピー
		//
		// Solverが衝突対象のBodyへアクセスするために使用する。
		// --------------------------------------------------------
		cc.a = bc.body_a;
		cc.b = bc.body_b;

		const contact& c = bc.c;

		// --------------------------------------------------------
		// Contact Normal（接触法線）
		//
		// 前提:
		//
		//   - 正規化済み（長さ1）
		//   - Body B → Body A の方向
		//
		// Projection時の位置補正方向として使用される。
		// --------------------------------------------------------
		cc.normal = c.normal;
		cc.contactPoint = c.position;

		// --------------------------------------------------------
		// Signed Distance（符号付き距離）
		//
		// めり込み量または分離距離を表す。
		//
		//   phi < 0
		//      → めり込み
		//         （補正が必要）
		//
		//   phi ≥ 0
		//      → すでに有効状態
		//
		// この値はProjection（押し戻し）処理で
		// 直接使用される。
		// --------------------------------------------------------
		cc.phi = c.phi;

		// --------------------------------------------------------
		// Stiffness（拘束の硬さ）
		//
		// Iterative Projection（反復補正）時に、
		// どれくらい強く拘束を適用するかを決定する。
		//
		//   1.0 → 硬い
		//   0.0 → 非常に柔らかい
		// --------------------------------------------------------
		cc.stiffness = stiffness;

		return cc;
	}
}