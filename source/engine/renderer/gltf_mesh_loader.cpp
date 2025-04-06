#include "gltf_mesh_loader.h"

#include "stb_image.h"
#include <iostream>

#include "initializers.h"
#include "types.h"
#include <glm/gtx/quaternion.hpp>

#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include <fastgltf/tools.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace
{
	VkFilter extract_filter(fastgltf::Filter filter)
	{
		switch (filter)
		{
			// nearest samplers
		case fastgltf::Filter::Nearest:
		case fastgltf::Filter::NearestMipMapNearest:
		case fastgltf::Filter::NearestMipMapLinear:
			return VK_FILTER_NEAREST;

			// linear samplers
		case fastgltf::Filter::Linear:
		case fastgltf::Filter::LinearMipMapNearest:
		case fastgltf::Filter::LinearMipMapLinear:
		default:
			return VK_FILTER_LINEAR;
		}
	}

	VkSamplerMipmapMode extract_mipmap_mode(fastgltf::Filter filter)
	{
		switch (filter)
		{
		case fastgltf::Filter::NearestMipMapNearest:
		case fastgltf::Filter::LinearMipMapNearest:
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;

		case fastgltf::Filter::NearestMipMapLinear:
		case fastgltf::Filter::LinearMipMapLinear:
		default:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
		}
	}
}

std::optional<std::shared_ptr<GltfMesh>> GltfMeshLoader::load_gltf_mesh()
{
	assert(!file_path.empty());;
	assert(device);
	assert(buffer_allocator);
	assert(image_allocator);
	assert(build_material);
	assert(upload_mesh);
	assert(white_image.image);
	assert(error_image.image);
	assert(default_sampler);

	fmt::print("Loading GLTF: {}", file_path);
	
	std::shared_ptr<GltfMesh> gltf_mesh_pointer = std::shared_ptr<GltfMesh>(new GltfMesh);

	gltf_mesh_pointer->m_device = device;
	gltf_mesh_pointer->m_buffer_allocator = buffer_allocator;
	gltf_mesh_pointer->m_image_allocator = image_allocator;
	gltf_mesh_pointer->m_error_image = error_image;	

	GltfMesh& gltf_mesh = *gltf_mesh_pointer.get();

	fastgltf::Parser parser{};

	constexpr auto gltf_options = fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble | fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;
	// fastgltf::Options::LoadExternalImages;

	fastgltf::GltfDataBuffer data;
	data.loadFromFile(file_path);

	fastgltf::Asset gltf;

	std::filesystem::path path = file_path;

	fastgltf::GltfType type = fastgltf::determineGltfFileType(&data);
	if (type == fastgltf::GltfType::glTF)
	{
		auto load = parser.loadGLTF(&data, path.parent_path(), gltf_options);
		if (load)
		{
			gltf = std::move(load.get());
		}
		else
		{
			std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
			return {};
		}
	}
	else if (type == fastgltf::GltfType::GLB)
	{
		auto load = parser.loadBinaryGLTF(&data, path.parent_path(), gltf_options);
		if (load)
		{
			gltf = std::move(load.get());
		}
		else
		{
			std::cerr << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
			return {};
		}
	}
	else
	{
		std::cerr << "Failed to determine glTF container" << std::endl;
		return {};
	}

	// we can estimate the descriptors we will need accurately
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 } };

	gltf_mesh.m_descriptor_pool.init(device, static_cast<uint32_t>(gltf.materials.size()), sizes);

	// load samplers

	for (fastgltf::Sampler& sampler : gltf.samplers)
	{
		VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr };
		sampl.maxLod = VK_LOD_CLAMP_NONE;
		sampl.minLod = 0;

		sampl.magFilter = extract_filter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
		sampl.minFilter = extract_filter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		sampl.mipmapMode = extract_mipmap_mode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		VkSampler newSampler;
		vkCreateSampler(device, &sampl, nullptr, &newSampler);

		gltf_mesh.m_samplers.push_back(newSampler);
	}

	// temporal arrays for all the objects to use while creating the GLTF data
	std::vector<std::shared_ptr<MeshAsset>> meshes;
	std::vector<std::shared_ptr<Node>> nodes;
	std::vector<AllocatedImage> images;
	std::vector<std::shared_ptr<MaterialInstance>> material_instances;

	// load textures

// load all textures
	for (fastgltf::Image& image : gltf.images)
	{
		std::optional<AllocatedImage> img = load_image(image_allocator, gltf, image);

		if (img.has_value())
		{
			images.push_back(*img);
			gltf_mesh.m_images[image.name.c_str()] = *img;
		}
		else
		{
			// we failed to load, so lets give the slot a default white texture to not
			// completely break loading
			images.push_back(error_image);
			std::cout << "gltf failed to load texture " << image.name << std::endl;
		}
	}

	// load materials

// create buffer to hold the material data
	gltf_mesh.m_material_data_buffer = buffer_allocator.create_buffer(sizeof(MetallicRoughnessMaterial::MaterialConstants) * gltf.materials.size(),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	int data_index = 0;

	MetallicRoughnessMaterial::MaterialConstants* sceneMaterialConstants = (MetallicRoughnessMaterial::MaterialConstants*)gltf_mesh.m_material_data_buffer.info.pMappedData;

	for (fastgltf::Material& material : gltf.materials)
	{
		std::shared_ptr<MaterialInstance> new_material_instance = std::make_shared<MaterialInstance>();
		material_instances.push_back(new_material_instance);
		gltf_mesh.m_material_instances[material.name.c_str()] = new_material_instance;

		MetallicRoughnessMaterial::MaterialConstants constants;
		constants.color_factors.x = material.pbrData.baseColorFactor[0];
		constants.color_factors.y = material.pbrData.baseColorFactor[1];
		constants.color_factors.z = material.pbrData.baseColorFactor[2];
		constants.color_factors.w = material.pbrData.baseColorFactor[3];

		constants.metal_rough_factors.x = material.pbrData.metallicFactor;
		constants.metal_rough_factors.y = material.pbrData.roughnessFactor;
		// write material parameters to buffer
		sceneMaterialConstants[data_index] = constants;

		MaterialPassType passType = MaterialPassType::MainColor;
		if (material.alphaMode == fastgltf::AlphaMode::Blend)
		{
			passType = MaterialPassType::Transparent;
		}

		MetallicRoughnessMaterial::MaterialResources material_resources;
		// default the material textures
		material_resources.color_image = white_image;
		material_resources.color_sampler = default_sampler;
		material_resources.metal_rough_image = white_image;
		material_resources.metal_rough_sampler = default_sampler;

		// set the uniform buffer for the material data
		material_resources.data_buffer = gltf_mesh.m_material_data_buffer.buffer;
		material_resources.data_buffer_offset = data_index * sizeof(MetallicRoughnessMaterial::MaterialConstants);
		// grab textures from gltf file
		if (material.pbrData.baseColorTexture.has_value())
		{
			size_t img = gltf.textures[material.pbrData.baseColorTexture.value().textureIndex].imageIndex.value();
			size_t sampler = gltf.textures[material.pbrData.baseColorTexture.value().textureIndex].samplerIndex.value();

			material_resources.color_image = images[img];
			material_resources.color_sampler = gltf_mesh.m_samplers[sampler];
		}
		// build material
		*new_material_instance = build_material(device, passType, material_resources, gltf_mesh.m_descriptor_pool);

		data_index++;
	}

	// load meshes

// use the same vectors for all meshes so that the memory doesnt reallocate as often
	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;

	for (fastgltf::Mesh& mesh : gltf.meshes)
	{
		std::shared_ptr<MeshAsset> new_mesh = std::make_shared<MeshAsset>();
		meshes.push_back(new_mesh);
		gltf_mesh.m_meshes[mesh.name.c_str()] = new_mesh;
		new_mesh->name = mesh.name;

		// clear the mesh arrays each mesh, we dont want to merge them by error
		indices.clear();
		vertices.clear();

		for (auto&& p : mesh.primitives)
		{
			GeoSurface new_surface;
			new_surface.start_index = (uint32_t)indices.size();
			new_surface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

			size_t initial_vtx = vertices.size();

			// load indexes
			{
				fastgltf::Accessor& index_accessor = gltf.accessors[p.indicesAccessor.value()];
				indices.reserve(indices.size() + index_accessor.count);

				fastgltf::iterateAccessor<std::uint32_t>(gltf, index_accessor,
					[&](std::uint32_t idx) {
						indices.push_back(idx + static_cast<std::uint32_t>(initial_vtx));
					});
			}

			// load vertex positions
			{
				fastgltf::Accessor& pos_accessor = gltf.accessors[p.findAttribute("POSITION")->second];
				vertices.resize(vertices.size() + pos_accessor.count);

				fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, pos_accessor,
					[&](glm::vec3 v, size_t index) {
						Vertex new_vtx;
						new_vtx.position = v;
						new_vtx.normal = { 1, 0, 0 };
						new_vtx.color = glm::vec4{ 1.f };
						new_vtx.uv_x = 0;
						new_vtx.uv_y = 0;
						vertices[initial_vtx + index] = new_vtx;
					});
			}

			// load vertex normals
			auto normals = p.findAttribute("NORMAL");
			if (normals != p.attributes.end())
			{
				fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
					[&](glm::vec3 v, size_t index) {
						vertices[initial_vtx + index].normal = v;
					});
			}

			// load UVs
			auto uv = p.findAttribute("TEXCOORD_0");
			if (uv != p.attributes.end())
			{
				fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
					[&](glm::vec2 v, size_t index) {
						vertices[initial_vtx + index].uv_x = v.x;
						vertices[initial_vtx + index].uv_y = v.y;
					});
			}

			// load vertex colors
			auto colors = p.findAttribute("COLOR_0");
			if (colors != p.attributes.end())
			{
				fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
					[&](glm::vec4 v, size_t index) {
						vertices[initial_vtx + index].color = v;
					});
			}

			// load material
			if (p.materialIndex.has_value())
			{
				new_surface.material_instance = material_instances[p.materialIndex.value()];
			}
			else
			{
				new_surface.material_instance = material_instances[0];
			}

			// compute bounds
			// loop the vertices of this surface, find min/max bounds
			glm::vec3 min_pos = vertices[initial_vtx].position;
			glm::vec3 max_pos = vertices[initial_vtx].position;

			for (size_t i = initial_vtx; i < vertices.size(); i++)
			{
				min_pos = glm::min(min_pos, vertices[i].position);
				max_pos = glm::max(max_pos, vertices[i].position);
			}
			// calculate origin and extents from the min/max, use extent lenght for radius
			new_surface.bounds.origin = (max_pos + min_pos) / 2.f;
			new_surface.bounds.extents = (max_pos - min_pos) / 2.f;
			new_surface.bounds.sphere_radius = glm::length(new_surface.bounds.extents);

			new_mesh->surfaces.push_back(new_surface);
		}

		new_mesh->mesh_buffers = upload_mesh(indices, vertices);
	}

	// load all nodes and their meshes

	for (fastgltf::Node& node : gltf.nodes)
	{
		std::shared_ptr<Node> new_node;

		// find if the node has a mesh, and if it does hook it to the mesh pointer and allocate it with the meshnode class
		if (node.meshIndex.has_value())
		{
			new_node = std::make_shared<MeshNode>();
			static_cast<MeshNode*>(new_node.get())->mesh = meshes[*node.meshIndex];
		}
		else
		{
			new_node = std::make_shared<Node>();
		}

		nodes.push_back(new_node);
		gltf_mesh.m_nodes[node.name.c_str()];

		std::visit(fastgltf::visitor
			{
				[&](fastgltf::Node::TransformMatrix matrix)
				{
					memcpy(&new_node->local_transform, matrix.data(), sizeof(matrix));
				},
				[&](fastgltf::Node::TRS transform)
				{
					glm::vec3 tl(transform.translation[0], transform.translation[1], transform.translation[2]);
					glm::quat rot(transform.rotation[3], transform.rotation[0], transform.rotation[1], transform.rotation[2]);
					glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);

					glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
					glm::mat4 rm = glm::toMat4(rot);
					glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);

					new_node->local_transform = tm * rm * sm;
				}
			},
			node.transform);
	}

	// nodes hierarchy

	for (int i = 0; i < gltf.nodes.size(); i++)
	{
		fastgltf::Node& node = gltf.nodes[i];
		std::shared_ptr<Node>& scene_node = nodes[i];

		for (auto& c : node.children)
		{
			scene_node->children.push_back(nodes[c]);
			nodes[c]->parent = scene_node;
		}
	}

	// find the top nodes, with no parents
	for (auto& node : nodes)
	{
		if (node->parent.lock() == nullptr)
		{
			gltf_mesh.m_top_nodes.push_back(node);
			node->refresh_transform(glm::mat4{ 1.f });
		}
	}
	return gltf_mesh_pointer;
}

std::optional<std::vector<std::shared_ptr<MeshAsset>>> GltfMeshLoader::load_gltf_meshes(std::filesystem::path file_path, UploadMesh upload_mesh)
{
	std::cout << "Loading GLTF: " << file_path << std::endl;

	fastgltf::GltfDataBuffer data;
	data.loadFromFile(file_path);

	constexpr auto gltf_options = fastgltf::Options::LoadGLBBuffers | fastgltf::Options::LoadExternalBuffers;

	fastgltf::Asset gltf;
	fastgltf::Parser parser{};

	auto load = parser.loadBinaryGLTF(&data, file_path.parent_path(), gltf_options);

	if (load)
	{
		gltf = std::move(load.get());
	}
	else
	{
		fmt::print("Failed to load glTF: {} \n", fastgltf::to_underlying(load.error()));
		return {};
	}

	std::vector<std::shared_ptr<MeshAsset>> meshes;

	// use the same vectors for all meshes so that the memory doesnt reallocate as
	// often
	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;

	for (fastgltf::Mesh& mesh : gltf.meshes)
	{
		MeshAsset new_mesh;

		new_mesh.name = mesh.name;

		// clear the mesh arrays each mesh, we dont want to merge them by error
		indices.clear();
		vertices.clear();

		for (auto&& p : mesh.primitives)
		{
			GeoSurface new_surface;
			new_surface.start_index = (uint32_t)indices.size();
			new_surface.count = (uint32_t)gltf.accessors[p.indicesAccessor.value()].count;

			size_t initial_vtx = vertices.size();

			// load indexes
			{
				fastgltf::Accessor& index_accessor = gltf.accessors[p.indicesAccessor.value()];
				indices.reserve(indices.size() + index_accessor.count);

				fastgltf::iterateAccessor<std::uint32_t>(gltf, index_accessor,
					[&](std::uint32_t idx)
					{
						indices.push_back(idx + static_cast<uint32_t>(initial_vtx));
					});
			}

			// load vertex positions
			{
				fastgltf::Accessor& pos_accessor = gltf.accessors[p.findAttribute("POSITION")->second];
				vertices.resize(vertices.size() + pos_accessor.count);

				fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, pos_accessor,
					[&](glm::vec3 v, size_t index)
					{
						Vertex new_vtx;
						new_vtx.position = v;
						new_vtx.normal = { 1, 0, 0 };
						new_vtx.color = glm::vec4{ 1.f };
						new_vtx.uv_x = 0;
						new_vtx.uv_y = 0;
						vertices[initial_vtx + index] = new_vtx;
					});
			}

			// load vertex normals
			auto normals = p.findAttribute("NORMAL");
			if (normals != p.attributes.end())
			{
				fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).second],
					[&](glm::vec3 v, size_t index) {
						vertices[initial_vtx + index].normal = v;
					});
			}

			// load UVs
			auto uv = p.findAttribute("TEXCOORD_0");
			if (uv != p.attributes.end())
			{
				fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).second],
					[&](glm::vec2 v, size_t index) {
						vertices[initial_vtx + index].uv_x = v.x;
						vertices[initial_vtx + index].uv_y = v.y;
					});
			}

			// load vertex colors
			auto colors = p.findAttribute("COLOR_0");
			if (colors != p.attributes.end())
			{
				fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).second],
					[&](glm::vec4 v, size_t index) {
						vertices[initial_vtx + index].color = v;
					});
			}
			new_mesh.surfaces.push_back(new_surface);
		}

		// display the vertex normals
		constexpr bool override_colors = true;
		if (override_colors)
		{
			for (Vertex& vtx : vertices)
			{
				vtx.color = glm::vec4(vtx.normal, 1.f);
			}
		}
		new_mesh.mesh_buffers = upload_mesh(indices, vertices);

		meshes.emplace_back(std::make_shared<MeshAsset>(std::move(new_mesh)));
	}

	return meshes;
}

std::optional<AllocatedImage> GltfMeshLoader::load_image(ImageAllocator imageAllocator, fastgltf::Asset & asset, fastgltf::Image & image)
{
	AllocatedImage new_image{};

	int width, height, num_channels;

	std::visit(
		fastgltf::visitor{
			[](auto& arg) {},
			[&](fastgltf::sources::URI& filePath)
			{
				assert(filePath.fileByteOffset == 0); // We don't support offsets with stbi.
				assert(filePath.uri.isLocalPath()); // We're only capable of loading
				// local files.

				const std::string path(filePath.uri.path().begin(),	filePath.uri.path().end()); // Thanks C++.
				unsigned char* data = stbi_load(path.c_str(), &width, &height, &num_channels, 4);
				if (data)
				{
					VkExtent3D image_size;
					image_size.width = width;
					image_size.height = height;
					image_size.depth = 1;

					new_image = imageAllocator.create_image(data, image_size, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

					stbi_image_free(data);
				}
			},
			[&](fastgltf::sources::Vector& vector)
			{
				unsigned char* data = stbi_load_from_memory(vector.bytes.data(), static_cast<int>(vector.bytes.size()),
					&width, &height, &num_channels, 4);
				if (data)
				{
					VkExtent3D image_size;
					image_size.width = width;
					image_size.height = height;
					image_size.depth = 1;

					new_image = imageAllocator.create_image(data, image_size, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

					stbi_image_free(data);
				}
			},
			[&](fastgltf::sources::BufferView& view)
			{
				auto& buffer_view = asset.bufferViews[view.bufferViewIndex];
				auto& buffer = asset.buffers[buffer_view.bufferIndex];

				std::visit(fastgltf::visitor
				{ // We only care about VectorWithMime here, because we
					// specify LoadExternalBuffers, meaning all buffers
					// are already loaded into a vector.
					[](auto& arg) {},
					[&](fastgltf::sources::Vector& vector)
					{
						unsigned char* data = stbi_load_from_memory(vector.bytes.data() + buffer_view.byteOffset, static_cast<int>(buffer_view.byteLength),
							&width, &height, &num_channels, 4);
						if (data)
						{
							VkExtent3D image_size;
							image_size.width = width;
							image_size.height = height;
							image_size.depth = 1;

							new_image = imageAllocator.create_image(data, image_size, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

							stbi_image_free(data);
						}
					}
				},
				buffer.data);
			},
		},
		image.data);

	// if any of the attempts to load the data failed, we havent written the image
	// so handle is null
	if (new_image.image == VK_NULL_HANDLE)
	{
		return {};
	}
	else
	{
		return new_image;
	}
}