#include "pch.h"
#include "VoxelVolume.h"

#include <queue>
#include <algorithm>

// ============================================================
// Voxel座標を線形インデックスへ変換
//
// メモリレイアウト:
//
//   index = x + nx * (y + ny * z)
//
// に従って、3次元座標を1次元配列のインデックスへ変換する。
// ============================================================

size_t voxel_volume::index(int x, int y, int z) const noexcept
{
	return static_cast<size_t>(x + nx * (y + ny * z));
}

// ============================================================
// Voxel Cellアクセス
// ============================================================

voxel_cell& voxel_volume::at(int x, int y, int z)
{
	return cells[index(x, y, z)];
}

const voxel_cell& voxel_volume::at(int x, int y, int z) const
{
	return cells[index(x, y, z)];
}

// ============================================================
// 指定Voxel座標がGrid内部か判定
// ============================================================

bool voxel_volume::inside(int x, int y, int z) const noexcept
{
	return
		x >= 0 && y >= 0 && z >= 0 &&
		x < nx && y < ny && z < nz;
}

// ============================================================
// 指定状態のVoxel数をカウント
// ============================================================

int voxel_volume::count(voxel_state state) const
{
	int result = 0;

	for (const auto& cell : cells)
	{
		if (cell.state == state)
			result++;
	}

	return result;
}

// ============================================================
// Voxel中心のWorld Space座標を計算
//
// Voxelの中心は:
//   origin + (index + 0.5) * cell_size
// で求められる。
// ============================================================

DirectX::XMFLOAT3 voxel_volume::center(int x, int y, int z) const
{
	return DirectX::XMFLOAT3
	{
		origin.x + (x + 0.5f) * cell_size,
		origin.y + (y + 0.5f) * cell_size,
		origin.z + (z + 0.5f) * cell_size
	};
}

// ============================================================
// Triangle Rasterization
//
// Triangleと重なるVoxelを
// boundary voxelとしてマークする。
// 現在の実装:
// TriangleのAABB（Axis Aligned Bounding Box）と
// 重なるVoxel全てをboundary扱いする。
// これは保守的（Conservative）な近似であり、
//   ○ 高速
//   × 境界が少し厚くなる
// という特徴がある。
// ============================================================

void rasterize_triangle(
	voxel_volume& grid,
	const DirectX::XMFLOAT3& v0,
	const DirectX::XMFLOAT3& v1,
	const DirectX::XMFLOAT3& v2)
{
	// --------------------------------------------------------
	// Triangle AABB計算
	// --------------------------------------------------------
	DirectX::XMFLOAT3 min_value =
	{
		std::min<float>({ v0.x, v1.x, v2.x }),
		std::min<float>({ v0.y, v1.y, v2.y }),
		std::min<float>({ v0.z, v1.z, v2.z })
	};

	DirectX::XMFLOAT3 max_value =
	{
		std::max<float>({ v0.x, v1.x, v2.x }),
		std::max<float>({ v0.y, v1.y, v2.y }),
		std::max<float>({ v0.z, v1.z, v2.z })
	};

	// --------------------------------------------------------
	// AABBをVoxel座標へ変換
	// --------------------------------------------------------
	int min_x = static_cast<int>((min_value.x - grid.origin.x) / grid.cell_size);
	int min_y = static_cast<int>((min_value.y - grid.origin.y) / grid.cell_size);
	int min_z = static_cast<int>((min_value.z - grid.origin.z) / grid.cell_size);

	int max_x = static_cast<int>((max_value.x - grid.origin.x) / grid.cell_size);
	int max_y = static_cast<int>((max_value.y - grid.origin.y) / grid.cell_size);
	int max_z = static_cast<int>((max_value.z - grid.origin.z) / grid.cell_size);

	// --------------------------------------------------------
	// Grid範囲へClamp
	// --------------------------------------------------------

	min_x = std::max<int>(0, min_x);
	min_y = std::max<int>(0, min_y);
	min_z = std::max<int>(0, min_z);

	max_x = std::min<int>(grid.nx - 1, max_x);
	max_y = std::min<int>(grid.ny - 1, max_y);
	max_z = std::min<int>(grid.nz - 1, max_z);

	
	Logger::Log(std::format(
		"tri min({},{},{}) max({},{},{}) ",
		min_value.x, min_value.y, min_value.z,
		max_value.x, max_value.y, max_value.z
		));

	Logger::Log(std::format(
		"grid.origin({},{},{}) cell({}) nx,ny,nz({},{},{}) ",
		grid.origin.x, grid.origin.y, grid.origin.z, grid.cell_size,
		grid.nx, grid.ny, grid.nz
	));

	Logger::Log(std::format(
		"voxel min({},{},{}) max({},{},{})  ",
		min_x, min_y, min_z,
		max_x, max_y, max_z
	));



    // --------------------------------------------------------
    // AABBに含まれるVoxelをBoundary化
    // --------------------------------------------------------
	for (int z = min_z; z <= max_z; z++)
		for (int y = min_y; y <= max_y; y++)
			for (int x = min_x; x <= max_x; x++)
			{
				grid.at(x, y, z).state = voxel_state::boundary;
			}
}

// ============================================================
// 外部空間Flood Fill
//
// アルゴリズム:
//
//   1. Grid境界上のunknown voxelを取得
//   2. BFSキューへ追加
//   3. 隣接unknown voxelへ拡張
//   4. 到達可能なVoxelをoutsideにする
//   5. 最後までunknownだったVoxelは
//      insideとみなす
//
// この手法は閉じたMeshで動作する。
// ============================================================

void flood_fill_outside(voxel_volume& grid)
{
	struct node { int x, y, z; };

	std::queue<node> q;

	// --------------------------------------------------------
	// Grid境界から探索開始
	// --------------------------------------------------------

	for (int x = 0; x < grid.nx; x++)
		for (int y = 0; y < grid.ny; y++)
			for (int z = 0; z < grid.nz; z++)
			{
				if (x == 0 || y == 0 || z == 0 ||
					x == grid.nx - 1 ||
					y == grid.ny - 1 ||
					z == grid.nz - 1)
				{
					voxel_cell& v = grid.at(x, y, z);

					if (v.state == voxel_state::unknown)
					{
						v.state = voxel_state::outside;
						q.push({ x,y,z });
					}
				}
			}

	// 6近傍探索
	const int dirs[6][3] =
	{
		{1,0,0},{-1,0,0},
		{0,1,0},{0,-1,0},
		{0,0,1},{0,0,-1}
	};

	// --------------------------------------------------------
	// Breadth First Search
	// --------------------------------------------------------

	while (!q.empty())
	{
		node n = q.front();
		q.pop();

		for (auto& d : dirs)
		{
			int nx = n.x + d[0];
			int ny = n.y + d[1];
			int nz = n.z + d[2];

			if (!grid.inside(nx, ny, nz))
				continue;

			voxel_cell& v = grid.at(nx, ny, nz);

			if (v.state == voxel_state::unknown)
			{
				v.state = voxel_state::outside;
				q.push({ nx,ny,nz });
			}
		}
	}

	// --------------------------------------------------------
	// 残ったunknownは内部空間
	// --------------------------------------------------------

	for (auto& v : grid.cells)
	{
		if (v.state == voxel_state::unknown)
			v.state = voxel_state::inside;
	}
}

// ============================================================
// AABBからVoxelサイズを計算
//
// 最長軸を resolution 分割することで、
// Cubic Voxelを維持する。
// ============================================================

float compute_voxel_size_from_resolution(
	const DirectX::XMFLOAT3& min_value,
	const DirectX::XMFLOAT3& max_value,
	int resolution)
{
	float dx = max_value.x - min_value.x;
	float dy = max_value.y - min_value.y;
	float dz = max_value.z - min_value.z;

	float longest = std::max<float>({ dx, dy, dz });

	return longest / resolution;
}

// ============================================================
// Mesh Voxelizationパイプライン
//
// 処理手順:
//   1. Voxel Grid生成
//   2. Triangle Rasterization
//   3. Flood Fill
//   4. Inside / Outside分類
//   5. Vertex → Voxelマッピング構築
// ============================================================

voxel_volume voxelize_mesh(
	const std::vector<DirectX::XMFLOAT3>& positions,
	const std::vector<UINT>& indices,
	const DirectX::XMFLOAT3& min_value,
	const DirectX::XMFLOAT3& max_value,
	float cell_size)
{
	voxel_volume grid;

	grid.cell_size = cell_size;

	// --------------------------------------------------------
	// Voxel Grid生成
	// --------------------------------------------------------

	// Mesh周囲へMargin追加
	//
	// Flood Fillが外部空間から開始できるようにする。

	float margin = cell_size * 2.0f;

	DirectX::XMFLOAT3 expanded_min =
	{
		min_value.x - margin,
		min_value.y - margin,
		min_value.z - margin
	};

	DirectX::XMFLOAT3 expanded_max =
	{
		max_value.x + margin,
		max_value.y + margin,
		max_value.z + margin
	};

	// Expanded AABBをGrid範囲にする
	grid.origin = expanded_min;

	grid.nx = static_cast<int>((expanded_max.x - expanded_min.x) / cell_size) + 1;
	grid.ny = static_cast<int>((expanded_max.y - expanded_min.y) / cell_size) + 1;
	grid.nz = static_cast<int>((expanded_max.z - expanded_min.z) / cell_size) + 1;

	grid.cells.resize(grid.nx * grid.ny * grid.nz);


	for (size_t i = 0; i < indices.size(); i += 3)
	{
		const auto& v0 = positions[indices[i + 0]];
		const auto& v1 = positions[indices[i + 1]];
		const auto& v2 = positions[indices[i + 2]];

		rasterize_triangle(grid, v0, v1, v2);
	}

	flood_fill_outside(grid);

	// --------------------------------------------------------
	// Vertex → Voxel Mapping構築
	//
	// 各頂点が所属するVoxelへ
	// Vertex Indexを登録する。
	// --------------------------------------------------------

	for (UINT i = 0; i < positions.size(); i++)
	{
		const auto& p = positions[i];

		int x = static_cast<int>((p.x - grid.origin.x) / grid.cell_size);
		int y = static_cast<int>((p.y - grid.origin.y) / grid.cell_size);
		int z = static_cast<int>((p.z - grid.origin.z) / grid.cell_size);

		if (!grid.inside(x, y, z))
			continue;

		grid.at(x, y, z).position_indices.push_back(i);
	}

	return grid;
}