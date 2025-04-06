#pragma once

#include "metallic_roughness_material.h"

struct GeoSurface
{
	uint32_t start_index;
	uint32_t count;
	Bounds bounds;
	std::shared_ptr<MaterialInstance> material_instance;
};

struct MeshAsset
{
	std::string name;

	std::vector<GeoSurface> surfaces;
	GpuMeshBuffers mesh_buffers;
};

struct MeshNode : public Node
{
	std::shared_ptr<MeshAsset> mesh;

	virtual void add_to_render_context(const glm::mat4& top_matrix, RenderContext& ctx) override;
};