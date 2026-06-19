#include "pch.h"
#include "PbdParticleDebugRenderer.h"
#include "Graphics/Core/Shader.h"

particle_debug_renderer::particle_debug_renderer(ID3D11Device* device, size_t particle_count)
{
	D3D11_BUFFER_DESC desc{};
	desc.ByteWidth = UINT(sizeof(DirectX::XMFLOAT3) * particle_count);
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	device->CreateBuffer(&desc, NULL, particle_buffer.ReleaseAndGetAddressOf());

	D3D11_INPUT_ELEMENT_DESC input_element_desc[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	HRESULT hr=CreateVsFromCSO(device, "./Data/Shaders/ParticleDebugRendererVS.cso", particle_debug_renderer_vs.ReleaseAndGetAddressOf(), particle_input_layout.ReleaseAndGetAddressOf(), input_element_desc, _countof(input_element_desc));
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	hr=CreatePsFromCSO(device, "./Data/Shaders/ParticleDebugRendererPS.cso", particle_debug_renderer_ps.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

}

void particle_debug_renderer::draw(ID3D11DeviceContext* immediate_context, const std::vector<PBD::PBDParticle>& particles)
{
	D3D11_MAPPED_SUBRESOURCE mapped{};
	immediate_context->Map(particle_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

	DirectX::XMFLOAT3* v = static_cast<DirectX::XMFLOAT3*>(mapped.pData);

	size_t particle_count = particles.size();
	for (size_t i = 0; i < particle_count; i++)
	{
		v[i] = particles[i].position;
	}
	immediate_context->Unmap(particle_buffer.Get(), 0);

	UINT stride = sizeof(DirectX::XMFLOAT3);
	UINT offset = 0;
	immediate_context->IASetVertexBuffers(0, 1, particle_buffer.GetAddressOf(), &stride, &offset);
	immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	immediate_context->VSSetShader(particle_debug_renderer_vs.Get(), NULL, 0);
	immediate_context->PSSetShader(particle_debug_renderer_ps.Get(), NULL, 0);
	immediate_context->IASetInputLayout(particle_input_layout.Get());
	immediate_context->Draw(static_cast<UINT>(particle_count), 0);
}
