#include "pch.h"
#include "DeformableModel.h"

#include <algorithm>

#include "Graphics/Core/Shader.h"
#include "Graphics/Resource/Texture.h"
#include "Engine/Utility/Win32Utils.h"
#include "Graphics/Core/GltfDxgiHelper.h"


deformable_model::deformable_model(ID3D11Device* device, const std::string& filename, float scale_factor)
{
	tinygltf::TinyGLTF tiny_gltf;
	tiny_gltf.SetImageLoader(NullLoadImage, nullptr);

	tinygltf::Model gltf_model;
	std::string error, warning;
	bool succeeded{ false };
	if (filename.find(".glb") != std::string::npos)
	{
		succeeded = tiny_gltf.LoadBinaryFromFile(&gltf_model, &error, &warning, filename.c_str());
	}
	else if (filename.find(".gltf") != std::string::npos)
	{
		succeeded = tiny_gltf.LoadASCIIFromFile(&gltf_model, &error, &warning, filename.c_str());
	}
	_ASSERT_EXPR_A(warning.empty(), warning.c_str());
	_ASSERT_EXPR_A(error.empty(), error.c_str());
	_ASSERT_EXPR_A(succeeded, L"Failed to load glTF file");

	parse_nodes(gltf_model);
	parse_meshes(gltf_model);
	parse_materials(gltf_model);
	parse_textures(gltf_model);


	DirectX::XMMATRIX transform = DirectX::XMMatrixScaling(-scale_factor, scale_factor, scale_factor);
	for (auto& position : positions)
	{
		DirectX::XMStoreFloat3(&position, DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&position), transform));
	}
	for (auto& non_position_vertex : non_position_vertices)
	{
		DirectX::XMStoreFloat3(&non_position_vertex.normal, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&non_position_vertex.normal), transform));
		float sigma = non_position_vertex.tangent.w;
		DirectX::XMStoreFloat4(&non_position_vertex.tangent, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat4(&non_position_vertex.tangent), transform));
		non_position_vertex.tangent.w = sigma;
	}


#if 0
	using namespace DirectX;

	// Computes an axis-aligned bounding box for the given mesh.
	XMVECTOR min_v = DirectX::g_XMFltMax;
	XMVECTOR max_v = DirectX::g_XMFltMin;
	for (auto& position : positions)
	{
		XMVECTOR v = DirectX::XMLoadFloat3(&position);

		min_v = XMVectorMin(min_v, v);
		max_v = XMVectorMax(max_v, v);

	}
	XMStoreFloat3(&min_value, min_v);
	XMStoreFloat3(&max_value, max_v);
#endif




	create_and_upload_resources(device);
}
void deformable_model::update_node_hierarchy_transforms()
{
	std::function<void(int, int)> traverse = [&](int parent_index, int node_index)
		{
			auto& node = nodes.at(node_index);
			DirectX::XMMATRIX S = DirectX::XMMatrixScaling(node.scale.x, node.scale.y, node.scale.z);
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(DirectX::XMVectorSet(node.rotation.x, node.rotation.y, node.rotation.z, node.rotation.w));
			DirectX::XMMATRIX T = DirectX::XMMatrixTranslation(node.translation.x, node.translation.y, node.translation.z);
			DirectX::XMStoreFloat4x4(&node.global_transform, S * R * T * (parent_index > -1 ? DirectX::XMLoadFloat4x4(&nodes.at(parent_index).global_transform) : DirectX::XMMatrixIdentity()));
			for (int child_index : node.children)
			{
				traverse(node_index, child_index);
			}
		};
	for (int node_index : scene_nodes)
	{
		traverse(-1/*parent_index*/, node_index);
	}
}
void deformable_model::parse_nodes(const tinygltf::Model& gltf_model)
{
	// Use default scene if specified, otherwise fall back to 0.
	int scene_index = (gltf_model.defaultScene >= 0) ? gltf_model.defaultScene : 0;
	scene_nodes = gltf_model.scenes[scene_index].nodes;

	for (const auto& gltf_node : gltf_model.nodes)
	{
		auto& node = nodes.emplace_back();
		node.name = gltf_node.name;
		node.mesh = gltf_node.mesh;
		node.children = gltf_node.children;

		if (!gltf_node.matrix.empty())
		{
			DirectX::XMFLOAT4X4 matrix;
			for (size_t row = 0; row < 4; row++)
			{
				for (size_t column = 0; column < 4; column++)
				{
					matrix(row, column) = static_cast<float>(gltf_node.matrix.at(4 * row + column));
				}
			}

			DirectX::XMVECTOR S, T, R;
			bool succeed = DirectX::XMMatrixDecompose(&S, &R, &T, DirectX::XMLoadFloat4x4(&matrix));
			_ASSERT_EXPR(succeed, L"Failed to decompose matrix.");

			DirectX::XMStoreFloat3(&node.scale, S);
			DirectX::XMStoreFloat4(&node.rotation, R);
			DirectX::XMStoreFloat3(&node.translation, T);
		}
		else
		{
			if (gltf_node.scale.size() > 0)
			{
				node.scale.x = static_cast<float>(gltf_node.scale.at(0));
				node.scale.y = static_cast<float>(gltf_node.scale.at(1));
				node.scale.z = static_cast<float>(gltf_node.scale.at(2));
			}
			if (gltf_node.translation.size() > 0)
			{
				node.translation.x = static_cast<float>(gltf_node.translation.at(0));
				node.translation.y = static_cast<float>(gltf_node.translation.at(1));
				node.translation.z = static_cast<float>(gltf_node.translation.at(2));
			}
			if (gltf_node.rotation.size() > 0)
			{
				node.rotation.x = static_cast<float>(gltf_node.rotation.at(0));
				node.rotation.y = static_cast<float>(gltf_node.rotation.at(1));
				node.rotation.z = static_cast<float>(gltf_node.rotation.at(2));
				node.rotation.w = static_cast<float>(gltf_node.rotation.at(3));
			}
		}
	}
	update_node_hierarchy_transforms();
}


void deformable_model::parse_meshes(const tinygltf::Model& gltf_model)
{
	meshes.resize(gltf_model.meshes.size());

	std::function<void(int)> traverse = [&](int node_index)->void
		{
			const node& node = nodes.at(node_index);
			if (node.mesh > -1)
			{
				const auto global_transform = DirectX::XMLoadFloat4x4(&node.global_transform); // Global transform of this node.

				const auto& gltf_mesh = gltf_model.meshes.at(node.mesh);

				auto& mesh = meshes.at(node.mesh);
				mesh.name = gltf_mesh.name;

				for (const tinygltf::Primitive& gltf_primitive : gltf_mesh.primitives)
				{
#if 1
					_ASSERT_EXPR(gltf_primitive.material > -1, L"This engine requires every glTF primitive to have a material.");
#endif
					auto& primitive = mesh.primitives.emplace_back();
					primitive.material = gltf_primitive.material;

					if (gltf_primitive.indices > -1)
					{
						const tinygltf::Accessor& gltf_accessor = gltf_model.accessors.at(gltf_primitive.indices);
						const tinygltf::BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);

						std::vector<UINT> cached_indices(gltf_accessor.count);
						const size_t vertex_offset = positions.size(); // Offset indices by current vertex count.

						if (gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
						{
							const BYTE* data = gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;
							for (size_t accessor_index = 0; accessor_index < gltf_accessor.count; ++accessor_index)
							{
								cached_indices.at(accessor_index) = static_cast<UINT>(data[accessor_index] + vertex_offset);
							}
						}
						else if (gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
						{
							const USHORT* data = reinterpret_cast<const USHORT*>(gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset);
							for (size_t accessor_index = 0; accessor_index < gltf_accessor.count; ++accessor_index)
							{
								cached_indices.at(accessor_index) = static_cast<UINT>(data[accessor_index] + vertex_offset);
							}
						}
						else if (gltf_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
						{
							const UINT* data = reinterpret_cast<const UINT*>(gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset);
							for (size_t accessor_index = 0; accessor_index < gltf_accessor.count; ++accessor_index)
							{
								cached_indices.at(accessor_index) = static_cast<UINT>(data[accessor_index] + vertex_offset);
							}
						}
						else
						{
							_ASSERT_EXPR(false, L"This index format is not supported.");
						}

						primitive.start_index_location = static_cast<UINT>(indices.size());
						primitive.index_count = static_cast<UINT>(cached_indices.size());
						indices.insert(indices.end(), cached_indices.begin(), cached_indices.end());

					}

					std::vector<DirectX::XMFLOAT3> cached_positions; // Temporary storage for positions.
					std::vector<vertex_non_position> cached_non_position_vertices; // Temporary storage for non-position attributes.
					if (gltf_primitive.attributes.size() > 0 && gltf_primitive.attributes.find("POSITION") != gltf_primitive.attributes.end())
					{
						cached_positions.resize(gltf_model.accessors.at(gltf_primitive.attributes.at("POSITION")).count);
						cached_non_position_vertices.resize(gltf_model.accessors.at(gltf_primitive.attributes.at("POSITION")).count);
					}
					else
					{
						// POSITION attribute is mandatory.
						continue;
					}

					for (std::map<std::string, int>::const_reference gltf_attribute : gltf_primitive.attributes)
					{
						const tinygltf::Accessor& gltf_accessor = gltf_model.accessors.at(gltf_attribute.second);
						const tinygltf::BufferView& gltf_buffer_view = gltf_model.bufferViews.at(gltf_accessor.bufferView);
						if (gltf_attribute.first == "POSITION")
						{
							// Copy positions into a tightly packed array.
							const unsigned char* s_data = gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;
							const size_t s_stride = gltf_accessor.ByteStride(gltf_buffer_view);
							const size_t d_stride = sizeof(DirectX::XMFLOAT3);
							const size_t count = gltf_accessor.count;
							unsigned char* d_data = reinterpret_cast<unsigned char*>(&cached_positions.data()->x);
							CopyStride<DirectX::XMFLOAT3>(d_data, d_stride, s_data, s_stride, count);
						}
						else
						{
							// Copy non-position vertex attributes into interleaved storage.
							const unsigned char* s_data = gltf_model.buffers.at(gltf_buffer_view.buffer).data.data() + gltf_buffer_view.byteOffset + gltf_accessor.byteOffset;
							const size_t s_stride = gltf_accessor.ByteStride(gltf_buffer_view);
							const size_t d_stride = sizeof(vertex_non_position);
							const size_t count = gltf_accessor.count;

							if (gltf_attribute.first == "NORMAL")
							{
								unsigned char* d_data = reinterpret_cast<unsigned char*>(&cached_non_position_vertices.data()->normal);
								CopyStride<DirectX::XMFLOAT3>(d_data, d_stride, s_data, s_stride, count);
							}
							else if (gltf_attribute.first == "TANGENT")
							{
								unsigned char* d_data = reinterpret_cast<unsigned char*>(&cached_non_position_vertices.data()->tangent);
								CopyStride<DirectX::XMFLOAT4>(d_data, d_stride, s_data, s_stride, count);
							}
							else if (gltf_attribute.first == "TEXCOORD_0")
							{
								unsigned char* d_data = reinterpret_cast<unsigned char*>(&cached_non_position_vertices.data()->texcoord_0);
								CopyStride<DirectX::XMFLOAT2>(d_data, d_stride, s_data, s_stride, count);
							}
							else
							{
								// Unsupported attribute is ignored.
							}
						}

						// Record attribute format for later input layout creation.
						primitive.attributes.emplace(gltf_attribute.first, ToDxgiFormat(gltf_accessor));
					}

					for (auto& cached_position : cached_positions)
					{
						// Apply node transform to vertex positions.
						DirectX::XMStoreFloat3(&cached_position, DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&cached_position), global_transform));
					}
					positions.insert(positions.end(), cached_positions.begin(), cached_positions.end());

					for (auto& cached_non_position_vertex : cached_non_position_vertices)
					{
						// Transform and normalize normals.
						DirectX::XMStoreFloat3(&cached_non_position_vertex.normal, DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&cached_non_position_vertex.normal), global_transform)));
						float sigma = cached_non_position_vertex.tangent.w; // Preserve handedness.
						cached_non_position_vertex.tangent.w = 0;
						// Transform and normalize tangents.
						DirectX::XMStoreFloat4(&cached_non_position_vertex.tangent, DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat4(&cached_non_position_vertex.tangent), global_transform)));
						cached_non_position_vertex.tangent.w = sigma;
					}
					non_position_vertices.insert(non_position_vertices.end(), cached_non_position_vertices.begin(), cached_non_position_vertices.end());
				}
			}
			for (int child_index : node.children)
			{
				// Traverse child nodes recursively.
				traverse(child_index);
			}
		};
	for (int node_index : scene_nodes)
	{
		// Start traversal from scene root nodes.
		traverse(node_index);
	}

	// Indices are stored as 32-bit unsigned integers.
	index_buffer_view.format = DXGI_FORMAT_R32_UINT;
	// Total size of the index buffer in bytes.
	index_buffer_view.size_in_bytes = static_cast<UINT>(indices.size() * sizeof(UINT));

	// Ensure position and non-position vertex streams are synchronized.
	_ASSERT_EXPR(positions.size() == non_position_vertices.size(), L"Positions and non_position_vertices must have the same number of elements.");
	// Vertex position buffer size and stride.
	position_buffer_view.size_in_bytes = static_cast<UINT>(positions.size() * sizeof(vertex_position));
	position_buffer_view.stride_in_bytes = static_cast<UINT>(sizeof(vertex_position));
	// Non-position vertex attribute buffer size and stride.
	non_position_buffer_view.size_in_bytes = static_cast<UINT>(non_position_vertices.size() * sizeof(vertex_non_position));
	non_position_buffer_view.stride_in_bytes = static_cast<UINT>(sizeof(vertex_non_position));
}

void deformable_model::parse_materials(const tinygltf::Model& gltf_model)
{
	for (const auto& gltf_material : gltf_model.materials)
	{
		auto& material = materials.emplace_back();

		material.emissive_factor[0] = static_cast<float>(gltf_material.emissiveFactor.at(0));
		material.emissive_factor[1] = static_cast<float>(gltf_material.emissiveFactor.at(1));
		material.emissive_factor[2] = static_cast<float>(gltf_material.emissiveFactor.at(2));

		material.alpha_mode = gltf_material.alphaMode == "OPAQUE" ? 0 : gltf_material.alphaMode == "MASK" ? 1 : gltf_material.alphaMode == "BLEND" ? 2 : 0;
		material.alpha_cutoff = static_cast<float>(gltf_material.alphaCutoff);
		material.double_sided = gltf_material.doubleSided ? 1 : 0;

		material.pbr_metallic_roughness.basecolor_factor[0] = static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor.at(0));
		material.pbr_metallic_roughness.basecolor_factor[1] = static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor.at(1));
		material.pbr_metallic_roughness.basecolor_factor[2] = static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor.at(2));
		material.pbr_metallic_roughness.basecolor_factor[3] = static_cast<float>(gltf_material.pbrMetallicRoughness.baseColorFactor.at(3));
		material.pbr_metallic_roughness.basecolor_texture.index = gltf_material.pbrMetallicRoughness.baseColorTexture.index;
		material.pbr_metallic_roughness.basecolor_texture.texcoord = gltf_material.pbrMetallicRoughness.baseColorTexture.texCoord;
		material.pbr_metallic_roughness.metallic_factor = static_cast<float>(gltf_material.pbrMetallicRoughness.metallicFactor);
		material.pbr_metallic_roughness.roughness_factor = static_cast<float>(gltf_material.pbrMetallicRoughness.roughnessFactor);
		material.pbr_metallic_roughness.metallic_roughness_texture.index = gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.index;
		material.pbr_metallic_roughness.metallic_roughness_texture.texcoord = gltf_material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord;

		material.normal_texture.index = gltf_material.normalTexture.index;
		material.normal_texture.texcoord = gltf_material.normalTexture.texCoord;
		material.normal_texture.scale = static_cast<float>(gltf_material.normalTexture.scale);

		material.occlusion_texture.index = gltf_material.occlusionTexture.index;
		material.occlusion_texture.texcoord = gltf_material.occlusionTexture.texCoord;
		material.occlusion_texture.strength = static_cast<float>(gltf_material.occlusionTexture.strength);

		material.emissive_texture.index = gltf_material.emissiveTexture.index;
		material.emissive_texture.texcoord = gltf_material.emissiveTexture.texCoord;
	}
}
void deformable_model::parse_textures(const tinygltf::Model& gltf_model)
{
	for (const auto& gltf_texture : gltf_model.textures)
	{
		auto& texture = textures.emplace_back();
		texture.name = gltf_texture.name;
		texture.source = gltf_texture.source;
	}
	for (const auto& gltf_image : gltf_model.images)
	{
		auto& image = images.emplace_back();
		image.name = gltf_image.name;
		image.width = gltf_image.width;
		image.height = gltf_image.height;
		image.component = gltf_image.component;
		image.bits = gltf_image.bits;
		image.pixel_type = gltf_image.pixel_type;
		image.mime_type = gltf_image.mimeType;
		image.uri = gltf_image.uri;
		image.as_is = gltf_image.as_is;

		if (gltf_image.bufferView > -1)
		{
			const tinygltf::BufferView& buffer_view = gltf_model.bufferViews.at(gltf_image.bufferView);
			const tinygltf::Buffer& buffer = gltf_model.buffers.at(buffer_view.buffer);
			const unsigned char* data = buffer.data.data() + buffer_view.byteOffset;
			image.cache_data.resize(buffer_view.byteLength);
			memcpy_s(image.cache_data.data(), image.cache_data.size(), data, buffer_view.byteLength);
		}
	}

}
void deformable_model::create_and_upload_resources(ID3D11Device* device)
{
	HRESULT hr = S_OK;


	// Create and upload vertex and index buffers on GPU
	if (index_buffer_view.size_in_bytes > 0)
	{
		index_buffer_view.buffer = static_cast<int>(buffers.size());

		D3D11_BUFFER_DESC buffer_desc = {};
		buffer_desc.ByteWidth = index_buffer_view.size_in_bytes;
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;
		D3D11_SUBRESOURCE_DATA subresource_data = {};
		subresource_data.pSysMem = indices.data();
		subresource_data.SysMemPitch = 0;
		subresource_data.SysMemSlicePitch = 0;
		hr = device->CreateBuffer(&buffer_desc, &subresource_data, buffers.emplace_back().GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
#if 1
		indices.clear();
#endif


	}

	if (non_position_buffer_view.size_in_bytes > 0)
	{
		non_position_buffer_view.buffer = static_cast<int>(buffers.size());

		D3D11_BUFFER_DESC buffer_desc = {};
		buffer_desc.ByteWidth = non_position_buffer_view.size_in_bytes;
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;
		D3D11_SUBRESOURCE_DATA subresource_data = {};
		subresource_data.pSysMem = non_position_vertices.data();
		subresource_data.SysMemPitch = 0;
		subresource_data.SysMemSlicePitch = 0;
		hr = device->CreateBuffer(&buffer_desc, &subresource_data, buffers.emplace_back().GetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
#if 1
		non_position_vertices.clear();
#endif
	}

	// -----------------------------------------------------------------------------
	// Create a structured buffer containing all materials used by this model,
	// and expose it to shaders via a Shader Resource View (SRV).
	//
	// The buffer is created as a GPU-only structured buffer (D3D11_USAGE_DEFAULT)
	// and is intended to be read from shaders as:
	//
	//     StructuredBuffer<material>
	//
	// Each material can be accessed in the shader by index (e.g. material_id).
	// -----------------------------------------------------------------------------

	// Prepare CPU-side material data for GPU upload.
	std::vector<material> material_buffer_data;
	material_buffer_data.reserve(materials.size());
	for (const auto& material : materials)
	{
		material_buffer_data.emplace_back(material);
	}

	// Create GPU structured buffer for material data.
	Microsoft::WRL::ComPtr<ID3D11Buffer> material_buffer;

	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = static_cast<UINT>(sizeof(material) * material_buffer_data.size());
	buffer_desc.StructureByteStride = sizeof(material);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;                 // GPU read-only
	buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;      // Bind as SRV
	buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;

	D3D11_SUBRESOURCE_DATA subresource_data{};
	subresource_data.pSysMem = material_buffer_data.data();

	// Upload material data to GPU.
	hr = device->CreateBuffer(
		&buffer_desc,
		&subresource_data,
		material_buffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// Create a Shader Resource View for the structured buffer.
	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc{};
	shader_resource_view_desc.Format = DXGI_FORMAT_UNKNOWN;  // Required for structured buffers
	shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	shader_resource_view_desc.Buffer.NumElements = static_cast<UINT>(material_buffer_data.size());

	hr = device->CreateShaderResourceView(
		material_buffer.Get(),
		&shader_resource_view_desc,
		material_buffer_shader_resource_view.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));


	// Create and upload textures on GPU
	for (image& image : images)
	{
		if (image.cache_data.size() > 0)
		{
			ID3D11ShaderResourceView* texture_resource_view = NULL;
			hr = LoadTextureFromMemory(device,image.cache_data.data(), image.cache_data.size(), &texture_resource_view);
			if (hr == S_OK)
			{
				texture_resource_views.emplace_back().Attach(texture_resource_view);
			}
			image.cache_data.clear();
		}
		else
		{
			const std::filesystem::path path(filename);
			ID3D11ShaderResourceView* shader_resource_view = NULL;
			std::wstring filename{ path.parent_path().concat(L"/").wstring() + std::wstring(image.uri.begin(), image.uri.end()) };
			hr = LoadTextureFromFile(device, filename.c_str(), &shader_resource_view, NULL);
			if (hr == S_OK)
			{
				texture_resource_views.emplace_back().Attach(shader_resource_view);
			}
		}
	}


	D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	hr=CreateVsFromCSO(device, "./Data/Shaders/DeformableModelVS.cso", vertex_shader.ReleaseAndGetAddressOf(), input_layout.ReleaseAndGetAddressOf(), input_element_desc, _countof(input_element_desc));
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	hr=CreatePsFromCSO(device, "./Data/Shaders/DeformableModelPS.cso", pixel_shader.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	buffer_desc.ByteWidth = sizeof(primitive_constants);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = 0;
	hr = device->CreateBuffer(&buffer_desc, nullptr, primitive_cbuffer.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

}
void deformable_model::draw(ID3D11DeviceContext* immediate_context, int instance_index, const DirectX::XMFLOAT4X4& deformation_rotation)
{
	immediate_context->VSSetShader(vertex_shader.Get(), NULL, 0);
	immediate_context->PSSetShader(pixel_shader.Get(), NULL, 0);
	immediate_context->IASetInputLayout(input_layout.Get());
	immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	immediate_context->PSSetShaderResources(0, 1, material_buffer_shader_resource_view.GetAddressOf());

	UINT stride[] = { sizeof(vertex_position) , sizeof(vertex_non_position) };
	UINT offset[] = { 0, 0 };
	ID3D11Buffer* vertex_buffers[] = { position_buffers.at(instance_index).Get(), buffers.at(non_position_buffer_view.buffer).Get() };
	immediate_context->IASetVertexBuffers(0, 2, vertex_buffers, stride, offset);

	for (const auto& mesh : meshes)
	{
		for (const auto& primitive : mesh.primitives)
		{
			primitive_constants primitive_data = {};
			primitive_data.material = primitive.material;
			primitive_data.has_tangent = primitive.has("TANGENT");
			primitive_data.skin = -1;
			primitive_data.deformation_rotation = deformation_rotation;
			immediate_context->UpdateSubresource(primitive_cbuffer.Get(), 0, 0, &primitive_data, 0, 0);
			immediate_context->VSSetConstantBuffers(0, 1, primitive_cbuffer.GetAddressOf());
			immediate_context->PSSetConstantBuffers(0, 1, primitive_cbuffer.GetAddressOf());

			const auto& material = materials.at(primitive.material);
			const int texture_indices[] =
			{
				material.pbr_metallic_roughness.basecolor_texture.index,
				material.pbr_metallic_roughness.metallic_roughness_texture.index,
				material.normal_texture.index,
				material.emissive_texture.index,
				material.occlusion_texture.index,
			};
			ID3D11ShaderResourceView* null_shader_resource_view{};
			std::vector<ID3D11ShaderResourceView*> shader_resource_views(_countof(texture_indices));
			for (int texture_index = 0; texture_index < shader_resource_views.size(); ++texture_index)
			{
				shader_resource_views.at(texture_index) = texture_indices[texture_index] > -1 ? texture_resource_views.at(textures.at(texture_indices[texture_index]).source).Get() : null_shader_resource_view;
			}
			immediate_context->PSSetShaderResources(1, static_cast<UINT>(shader_resource_views.size()), shader_resource_views.data());

			if (index_buffer_view.buffer > -1)
			{
				immediate_context->IASetIndexBuffer(buffers.at(index_buffer_view.buffer).Get(), index_buffer_view.format, 0);
				immediate_context->DrawIndexed(primitive.index_count, primitive.start_index_location, 0);
			}
			else
			{
				immediate_context->Draw(position_buffer_view.size_in_bytes / position_buffer_view.stride_in_bytes, 0);
			}
		}
	}
}