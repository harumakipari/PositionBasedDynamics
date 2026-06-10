#pragma once
#include <vector>

namespace PBD
{
	struct PBDWorld;
	struct PBDParticle;
	struct ShapeMatchingBody;
	struct ContactConstraint;
	struct FrictionConstraint;
	struct DistanceConstraint;

	/*
		Position-Based Dynamics (PBD) の
		コアソルバ。

		シミュレーションパイプラインを実装する。

		このソルバは
		シミュレーションを1ステップ進め、
		Particle の position を直接操作する。

		シミュレーションの流れ:

		  1. 外力積分（velocity更新）
		  2. position予測
		  3. Constraint生成（接触・摩擦など）
		  4. Constraint反復解決（Gauss-Seidel）
		  5. 修正後positionからvelocity再構築

		Notes:
		- Position が主状態
		- Velocity は補助情報
		- Constraint は position を直接修正して解決する

		References:
		  Muller et al. - "Position Based Dynamics"
	*/
	class Solver
	{
	public:

		// Constraint解決に使用する
		// Gauss-Seidel反復回数。
		//
		// 増やすほど:
		// - 剛性向上
		// - 収束向上
		//
		// ただしコスト増加。
		int solver_iterations = 5;

		void step(PBDWorld& world, float dt);

	private:

		std::vector<ContactConstraint> collision_constraints;
		std::vector<FrictionConstraint> friction_constraints;

		// --------------------------------------------------------
		// integrate
		//
		// 実行内容:
		//   1. 外力による velocity 更新
		//   2. Constraint未適用 position 予測
		//
		// PBDの「予測ステージ」
		// --------------------------------------------------------
		void integrate(PBDWorld& world, float dt);

		// --------------------------------------------------------
		// generate_contacts
		//
		// Collision検出し、
		// SolverConstraintへ変換する。
		//
		// 出力:
		//   - collision_constraints
		//   - friction_constraints
		//
		// CollisionDetection と Solver の橋渡し
		// --------------------------------------------------------

		virtual void generate_contacts(PBDWorld& world);

		// --------------------------------------------------------
		// 摩擦Constraint
		//
		// 接触点での接線方向運動を
		// Coulomb摩擦近似で解決する。
		//
		// 制約:
		//  |Δx_t| ≤ μ |Δx_n|
		// --------------------------------------------------------
		void project_friction_constraint(PBDWorld& world, const FrictionConstraint& fc);

		// --------------------------------------------------------
		// 接触Constraint
		//
		// 法線方向へ押し戻し、
		// めり込みを解消する。
		//
		// 制約:
		//   φ ≥ 0
		//   φ は 符号付き距離。
		// --------------------------------------------------------
		void project_contact_constraint(PBDWorld& world, const ContactConstraint& cc);

		// --------------------------------------------------------
		// DistanceConstraint
		//
		// 2Particle間距離を維持する。
		//
		// Constraint:
		//　C(x1, x2) = |x1 - x2| - rest_length = 0
		//
		// Gauss-Seidelで修正。
		// --------------------------------------------------------

		void project_distance_constraint(PBDWorld& world, const DistanceConstraint& c);
	};
}