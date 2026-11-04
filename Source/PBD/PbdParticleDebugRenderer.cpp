#include "pch.h"
#include "PbdParticleDebugRenderer.h"
#include "Graphics/Core/Shader.h"

particle_debug_renderer::particle_debug_renderer(ID3D11Device* device, size_t particle_count)
{
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = UINT(sizeof(ParticleDebugVertex) * particle_count);
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&desc, NULL, particle_buffer.ReleaseAndGetAddressOf());

    D3D11_INPUT_ELEMENT_DESC input_element_desc[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "SIZE", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    HRESULT hr = CreateVsFromCSO(device, "./Data/Shaders/ParticleDebugRendererVS.cso", particle_debug_renderer_vs.ReleaseAndGetAddressOf(), particle_input_layout.ReleaseAndGetAddressOf(), input_element_desc, _countof(input_element_desc));
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
    hr = CreateGsFromCSO(device, "./Data/Shaders/ParticleDebugRendererGS.cso", particle_debug_renderer_gs.ReleaseAndGetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
    hr = CreatePsFromCSO(device, "./Data/Shaders/ParticleDebugRendererPS.cso", particle_debug_renderer_ps.ReleaseAndGetAddressOf());
    _ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

void particle_debug_renderer::draw(
    ID3D11DeviceContext* immediate_context,
    const std::vector<PBD::PBDParticle>& particles,
    bool show_surface_particles,
    bool show_interior_particles,
    float surface_particle_size,
    float interior_particle_size,
    const DirectX::XMFLOAT3& interior_particle_color)
{
    D3D11_MAPPED_SUBRESOURCE mapped{};
    immediate_context->Map(particle_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

    ParticleDebugVertex* v = static_cast<ParticleDebugVertex*>(mapped.pData);

    size_t visible_particle_count = 0;
    for (const PBD::PBDParticle& particle : particles)
    {
        // Keep future particle categories visible; only the two current
        // surface/interior classifications are controlled by these filters.
        if ((particle.voxelState == voxel_state::outside && !show_surface_particles) ||
            (particle.voxelState == voxel_state::inside && !show_interior_particles))
        {
            continue;
        }

        ParticleDebugVertex& debug_vertex = v[visible_particle_count++];
        debug_vertex.position = particle.position;
        debug_vertex.color = { 1, 1, 1 };
        debug_vertex.size = surface_particle_size;

        switch (particle.voxelState)
        {
        case voxel_state::unknown:
            break;
        case voxel_state::outside:
            debug_vertex.color = { 1,1,1 };
            break;
        case voxel_state::boundary:
            debug_vertex.color = { 1,0,1 };
            break;
        case voxel_state::inside:
            debug_vertex.color = interior_particle_color;
            debug_vertex.size = interior_particle_size;
            break;
        }
    }
    immediate_context->Unmap(particle_buffer.Get(), 0);

    UINT stride = sizeof(ParticleDebugVertex);
    UINT offset = 0;
    immediate_context->IASetVertexBuffers(0, 1, particle_buffer.GetAddressOf(), &stride, &offset);
    immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
    immediate_context->VSSetShader(particle_debug_renderer_vs.Get(), NULL, 0);
    immediate_context->GSSetShader(particle_debug_renderer_gs.Get(), NULL, 0);
    immediate_context->PSSetShader(particle_debug_renderer_ps.Get(), NULL, 0);
    immediate_context->IASetInputLayout(particle_input_layout.Get());
    immediate_context->Draw(static_cast<UINT>(visible_particle_count), 0);
    immediate_context->GSSetShader(nullptr, NULL, 0);
}
