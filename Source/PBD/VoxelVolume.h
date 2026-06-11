#pragma once

#include <vector>
#include <windows.h>
#include <DirectXMath.h>

// ============================================================
// Voxel分類状態
//
// unknown
//   まだ分類されていないVoxel。
//
// outside
//   メッシュ外部と連結している空間Voxel。
//
// boundary
//   メッシュ表面と交差しているVoxel。
//
// inside
//   閉じたメッシュ内部に存在するVoxel。
//
// 一般的なVoxelizationパイプライン:
//
//   1. Mesh TriangleをRasterize
//        → boundary voxel生成
//
//   2. Grid境界からFlood Fill
//        → outside voxel分類
//
//   3. 残ったunknown voxel
//        → inside分類
//
// NOTE:
//   この手法は
//   「閉じた manifold mesh」であることを前提とする。
// ============================================================

enum class voxel_state
{
    unknown,
    outside,
    boundary,
    inside
};

// ============================================================
// voxel_cell
//
// Grid内の単一Voxel Cellを表す構造体。
//
// position_indices
//   このVoxel内部に存在する
//   Mesh Vertex Indexを保持する。
//
// 簡易Spatial Acceleration Structureとして機能する。
//
// 主な用途:
//
//   • 最近傍頂点探索
//   • Particle Sampling
//   • Density Estimation
// ============================================================

struct voxel_cell
{
    voxel_state state = voxel_state::unknown;

    // Voxel中心座標（World Space）
    DirectX::XMFLOAT3 center;

    // このVoxel内部に存在するVertex Index
    std::vector<UINT> position_indices;

};

// ============================================================
// voxel_volume
//
// MeshのAABBを覆う
// Uniform 3D Voxel Gridを表す。
//
// メモリレイアウト:
//
//   linear index = x + nx * (y + ny * z)
//
// Grid座標:
//
//   x ∈ [0, nx)
//
//   y ∈ [0, ny)
//
//   z ∈ [0, nz)
//
// World Space位置:
//
//   world = origin + voxel_index * cell_size
//
// ============================================================

struct voxel_volume
{
    // Gridサイズ
    int nx = 0;
    int ny = 0;
    int nz = 0;

    // 1Voxelのサイズ
    float cell_size = 0.0f;

    // Voxel (0,0,0) のWorld Space原点
    DirectX::XMFLOAT3 origin{};

    // Voxel Cellの線形配列
    std::vector<voxel_cell> cells;

    // ------------------------------------------------------------
    // 3D Voxel座標 → 線形配列Indexへ変換
    // ------------------------------------------------------------
    size_t index(int x, int y, int z) const noexcept;

    // ------------------------------------------------------------
    // Voxel Cellアクセス
    // ------------------------------------------------------------
    voxel_cell& at(int x, int y, int z);
    const voxel_cell& at(int x, int y, int z) const;

    // ------------------------------------------------------------
    // 指定Voxel座標がGrid内部か判定
    // ------------------------------------------------------------
    bool inside(int x, int y, int z) const noexcept;

    // ------------------------------------------------------------
    // 指定状態のVoxel数をカウント
    // ------------------------------------------------------------
    int count(voxel_state state) const;

    // ------------------------------------------------------------
    // Voxel中心のWorld Space座標を計算
    // ------------------------------------------------------------
    DirectX::XMFLOAT3 center(int x, int y, int z) const;
};

// ============================================================
// TriangleをVoxel GridへRasterizeする。
//
// Triangleと重なったVoxelをboundary voxelとしてマークする。
// ============================================================

void rasterize_triangle(
    voxel_volume& grid,
    const DirectX::XMFLOAT3& v0,
    const DirectX::XMFLOAT3& v1,
    const DirectX::XMFLOAT3& v2);

// ============================================================
// outside voxel分類用Flood Fillアルゴリズム。
//
// Grid境界から開始し、unknown空間へ拡張していく。
// ============================================================

void flood_fill_outside(voxel_volume& grid);

// ============================================================
// AABBと目標Resolutionから
// Cubic Voxel Sizeを計算する。
//
// AABBの最長軸を`resolution` 個に分割することで、Cubic Voxelを維持する。
// ============================================================

float compute_voxel_size_from_resolution(
    const DirectX::XMFLOAT3& min_value,
    const DirectX::XMFLOAT3& max_value,
    int resolution);

// ============================================================
// Mesh Voxelizationメイン処理。
//
// パイプライン:
//
//   1. Mesh AABBを覆うVoxel Grid生成
//   2. Triangle Rasterization → boundary voxel生成
//   3. Flood Fill → outside分類
//   4. 残ったVoxel → inside分類
//   5. Vertex → Voxel Spatial Mapping構築
//
// ============================================================

voxel_volume voxelize_mesh(
    const std::vector<DirectX::XMFLOAT3>& positions,
    const std::vector<UINT>& indices,
    const DirectX::XMFLOAT3& min_value,
    const DirectX::XMFLOAT3& max_value,
    float cell_size);