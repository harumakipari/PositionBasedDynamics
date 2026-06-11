#pragma once

#include <vector>
#include <d3d11.h>
#include <wrl.h>
#include <directxmath.h>

#include "PbdParticle.h"

class particle_debug_renderer
{
public:
	particle_debug_renderer(ID3D11Device* device, size_t particle_count);
	void draw(ID3D11DeviceContext* immediate_context, const std::vector<PBD::PBDParticle>& particles);

private:
	Microsoft::WRL::ComPtr<ID3D11Buffer> particle_buffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> particle_debug_renderer_vs;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> particle_debug_renderer_ps;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> particle_input_layout;
};