#pragma once

#include "PbdWorld.h"
#include "PbdParticle.h"
#include "PbdCollision.h"
#include "ShapeMatchingBody.h"
#include "PbdDistanceConstraint.h"
#include "PbdContactConstraint.h"
#include "PbdFrictionConstraint.h"

namespace PBD
{
    /*
    Position-Based Dynamics (PBD)

    この名前空間には、
    Position-Based Dynamics (PBD) シミュレーションを実装するための
    中核クラスおよび構造体が含まれる。

    主な概念:
    - particle :
        シミュレーションの基本単位。
        点質量や頂点を表現する。

    - solver :
        反復的な位置修正を行う制約ソルバ。
        （Gauss-Seidel 法スタイル）

    - world :
        全 Particle、Constraint、
        シミュレーションパラメータを保持する。
        Simulation や PhysicsScene と呼ばれることもある。

    - collision :
        Shape や他の Particle との衝突判定および
        衝突応答を処理する。

    - shape_matching_body / shape_cluster :
        変形可能物体のための
        Shape Matching 制約を実装する。

    - distance_constraint /
      contact_constraint /
      friction_constraint :
        Particle 間の位置関係を維持するために使用される
        Constraint（制約）の種類。

    参考文献:
    1. Muller, M., Heidelberger, B., Hennix, M., & Ratcliff, J.
       「Position Based Dynamics」
       Journal of Visual Communication
       and Image Representation, 2007.

    2. Muller, M., Chentanez, N., & Kim, T.
       「Real-Time Physics Simulation in Games」
       SIGGRAPH Courses, 2010.

    3. Muller, M.
       「Unified Particle Physics for Real-Time Applications」
       ACM SIGGRAPH, 2009.

    PBD は特に、
    ゲームやインタラクティブシミュレーションのような
    リアルタイムアプリケーションに適している。

    その理由は:
    - 安定している
    - 高速
    - 実装しやすい

    さらに、
    力(force)ではなく位置(position)に対して
    直接制約を適用することで、
    安定したシミュレーションを実現できるためである。
*/
}
