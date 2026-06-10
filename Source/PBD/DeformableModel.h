#pragma once

#include <d3d11.h>
#include <wrl.h>
#include <directxmath.h>

#include <vector>
#include <unordered_map>
#include <string>


#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"


#include "PbdParticle.h"
#include "IndexRange.h"
#include "Engine/Utility/Win32Utils.h"

class deformable_model
{
	std::string filename;
public:
	deformable_model(ID3D11Device* device, const std::string& filename, float scale_factor);

#if 0
	// Bounding box(AABB)
	DirectX::XMFLOAT3 min_value;
	DirectX::XMFLOAT3 max_value;
#endif

	// indices of root node.
	std::vector<int> scene_nodes;

	struct node
	{
		std::string name;
		int mesh = -1;  // index of mesh referenced by this node

		std::vector<int> children; // An array of indices of child nodes of this node

		// Local transforms
		DirectX::XMFLOAT4 rotation{ 0, 0, 0, 1 };
		DirectX::XMFLOAT3 scale{ 1, 1, 1 };
		DirectX::XMFLOAT3 translation{ 0, 0, 0 };

		DirectX::XMFLOAT4X4 global_transform{ 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
	};
	std::vector<node> nodes;

	std::vector<UINT> indices;

	using vertex_position = DirectX::XMFLOAT3;
	std::vector<vertex_position> positions;

	struct vertex_non_position
	{
		DirectX::XMFLOAT3 normal = { 0, 0, 1 };
		DirectX::XMFLOAT4 tangent = { 1, 0, 0, 1 };
		DirectX::XMFLOAT2 texcoord_0 = { 0, 0 };
	};
	std::vector<vertex_non_position> non_position_vertices;

	struct index_buffer_view
	{
		int buffer = -1;
		UINT size_in_bytes = 0;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	};
	index_buffer_view index_buffer_view;

	struct vertex_buffer_view
	{
		int buffer = -1;
		UINT size_in_bytes = 0;
		UINT stride_in_bytes = 0;
	};
	vertex_buffer_view position_buffer_view;
	vertex_buffer_view non_position_buffer_view;

	std::vector<Microsoft::WRL::ComPtr<ID3D11Buffer>> buffers; // index_buffers, non_position_buffers
	std::vector<Microsoft::WRL::ComPtr<ID3D11Buffer>> position_buffers;

	int allocate_instance_buffer(ID3D11Device* device)
	{
		D3D11_BUFFER_DESC buffer_desc = {};
		buffer_desc.ByteWidth = position_buffer_view.size_in_bytes;
		buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;
#if 0
		D3D11_SUBRESOURCE_DATA subresource_data = {};
		subresource_data.pSysMem = positions.data();
		subresource_data.SysMemPitch = 0;
		subresource_data.SysMemSlicePitch = 0;
		HRESULT hr = device->CreateBuffer(&buffer_desc, &subresource_data, position_buffers.emplace_back().GetAddressOf());
#else
		HRESULT hr = device->CreateBuffer(&buffer_desc, NULL, position_buffers.emplace_back().GetAddressOf());

#endif
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		return static_cast<int>(position_buffers.size()) - 1;
	}

	struct primitive
	{
		int material;

		UINT index_count;
		UINT start_index_location;

		std::unordered_map<std::string, DXGI_FORMAT> attributes;

		bool has(const char* attribute) const
		{
			return attributes.find(attribute) != attributes.end();
		}
	};
	struct mesh
	{
		std::string name;
		std::vector<primitive> primitives;
	};
	std::vector<mesh> meshes;

	struct texture_info
	{
		int index = -1; // required.
		int texcoord = 0; // The set index of texture's TEXCOORD attribute used for texture coordinate mapping.
	};
	// https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#reference-material-normaltextureinfo
	struct normal_texture_info
	{
		int index = -1;  // required
		int texcoord = 0;    // The set index of texture's TEXCOORD attribute used for texture coordinate mapping.
		float scale = 1;    // scaledNormal = normalize((<sampled normal texture value> * 2.0 - 1.0) * vec3(<normal scale>, <normal scale>, 1.0))
	};
	// https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#reference-material-occlusiontextureinfo
	struct occlusion_texture_info
	{
		int index = -1;   // required
		int texcoord = 0;     // The set index of texture's TEXCOORD attribute used for texture coordinate mapping.
		float strength = 1;  // A scalar parameter controlling the amount of occlusion applied. A value of `0.0` means no occlusion. A value of `1.0` means full occlusion. This value affects the final occlusion value as: `1.0 + strength * (<sampled occlusion texture value> - 1.0)`.
	};
	// https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#reference-material-pbrmetallicroughness
	struct pbr_metallic_roughness
	{
		float basecolor_factor[4] = { 1, 1, 1, 1 };  // len = 4. default [1,1,1,1]
		texture_info basecolor_texture;
		float metallic_factor = 1;   // default 1
		float roughness_factor = 1;  // default 1
		texture_info metallic_roughness_texture;
	};
	struct material
	{
		float emissive_factor[3] = { 0, 0, 0 };  // length 3. default [0, 0, 0]
		int alpha_mode = 0;	// "OPAQUE" : 0, "MASK" : 1, "BLEND" : 2
		float alpha_cutoff = 0.5f; // default 0.5
		int double_sided = 0; // default false;

		pbr_metallic_roughness pbr_metallic_roughness;

		normal_texture_info normal_texture;
		occlusion_texture_info occlusion_texture;
		texture_info emissive_texture;
	};
	std::vector<material> materials;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> material_buffer_shader_resource_view;

	struct texture
	{
		std::string name;
		int source = -1;
	};
	std::vector<texture> textures;

	struct image
	{
		std::string name;
		int width = -1;
		int height = -1;
		int component = -1;
		int bits = -1;			// bit depth per channel. 8(byte), 16 or 32.
		int pixel_type = -1;	// pixel type(TINYGLTF_COMPONENT_TYPE_***). usually UBYTE(bits = 8) or USHORT(bits = 16)
		std::string mime_type;	// (required if no uri) ["image/jpeg", "image/png", "image/bmp", "image/gif"]
		std::string uri;		// (required if no mimeType) uri is not decoded(e.g. whitespace may be represented as %20)

		// When this flag is true, data is stored to `image` in as-is format(e.g. jpeg
		// compressed for "image/jpeg" mime) This feature is good if you use custom
		// image loader function. (e.g. delayed decoding of images for faster glTF
		// parsing) Default parser for Image does not provide as-is loading feature at
		// the moment. (You can manipulate this by providing your own LoadImageData
		// function)
		bool as_is = false;

		std::vector<unsigned char> cache_data;
	};
	std::vector<image> images;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> texture_resource_views;

public:
	void update_node_hierarchy_transforms();
	void draw(ID3D11DeviceContext* immediate_context, int body_index, const DirectX::XMFLOAT4X4& deformation_rotation);

	ID3D11Buffer* get_position_buffer() const
	{
		return buffers[position_buffer_view.buffer].Get();
	}

	void update_vertex_buffer(ID3D11DeviceContext* immediate_context, int instance_index, const DirectX::XMFLOAT3* positions, size_t stride)
	{
		size_t count = position_buffer_view.size_in_bytes / sizeof(vertex_position);

		D3D11_MAPPED_SUBRESOURCE mapped = {};
		ID3D11Buffer* vertex_buffer = position_buffers[instance_index].Get();
		HRESULT hr = immediate_context->Map(
			vertex_buffer,
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mapped);

		_ASSERT_EXPR(
			SUCCEEDED(hr),
			(L"ID3D11DeviceContext::Map() failed in update_vertex_buffer. "
				L"Failed to map vertex buffer for deformation update.")
		);

		DirectX::XMFLOAT3* v = reinterpret_cast<DirectX::XMFLOAT3*>(mapped.pData);
		const std::uint8_t* p = reinterpret_cast<const std::uint8_t*>(positions);
		for (size_t i = 0; i < count; ++i)
		{
			v[i] = *(reinterpret_cast<const DirectX::XMFLOAT3*>(p));
			p += stride;
		}

		immediate_context->Unmap(vertex_buffer, 0);
	}
private:
	void parse_nodes(const tinygltf::Model& gltf_model);
	void parse_meshes(const tinygltf::Model& gltf_model);
	void parse_materials(const tinygltf::Model& gltf_model);
	void parse_textures(const tinygltf::Model& gltf_model);

	void create_and_upload_resources(ID3D11Device* device);



	Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;
	struct primitive_constants
	{
		DirectX::XMFLOAT4X4 deformation_rotation;
		int material = -1;
		int has_tangent = 0;
		int skin = -1;
		int padding;
	};
	Microsoft::WRL::ComPtr<ID3D11Buffer> primitive_cbuffer;
};