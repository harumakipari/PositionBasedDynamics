#pragma once

#include <vector>
#include <d3d11.h>
#include <wrl.h>
#include <directxmath.h>

#include "PbdParticle.h"

class particle_debug_renderer
{
	struct ParticleDebugVertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 color;
		float size;
	};
public:
	particle_debug_renderer(ID3D11Device* device, size_t particle_count);
	void draw(
		ID3D11DeviceContext* immediate_context,
		const std::vector<PBD::PBDParticle>& particles,
		bool show_surface_particles,
		bool show_interior_particles,
		float surface_particle_size,
		float interior_particle_size,
		const DirectX::XMFLOAT3& interior_particle_color);

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> particle_buffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> particle_debug_renderer_vs;
	Microsoft::WRL::ComPtr<ID3D11GeometryShader> particle_debug_renderer_gs;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> particle_debug_renderer_ps;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> particle_input_layout;
};
