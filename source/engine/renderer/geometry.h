#pragma once

#include <glm/glm.hpp>
#include <concepts>
#include "types.h"

namespace Geometry
{
	void create_sphere(std::vector<Vertex>& out_vertices, float radius, glm::vec3 center, uint32_t resolution, glm::vec4 color);

	void create_piramyd(std::vector<Vertex>& out_vertices, std::vector<uint32_t>& out_indices, float base, float height, glm::vec4 color);
	void create_piramyd(std::vector<Vertex>& out_vertices, float base, float height, glm::vec4 color);

	void create_box(std::vector<Vertex>& out_vertices, std::vector<uint32_t>& out_indices, glm::vec3 dimensions, glm::vec4 color);
	void create_box(std::vector<Vertex>& out_vertices, glm::vec3 dimensions, glm::vec4 color);

	void create_arrow(std::vector<Vertex>& out_vertices, std::vector<uint32_t>& out_indices, glm::vec4 color);
	void create_arrow(std::vector<Vertex>& out_vertices, glm::vec4 color);

	void create_gizmo(std::vector<Vertex>& out_vertices, std::vector<uint32_t>& out_indices);
	void create_gizmo(std::vector<Vertex>& out_vertices);

	template<std::integral TIndex>
	std::vector<Vertex> convert_to_unique_vertices(const std::vector<Vertex>& vertices, const std::vector<TIndex> indices)
	{
		std::vector<Vertex> res;
		res.reserve(indices.size());

		for (const auto index : indices)
		{
			res.push_back(vertices[index]);
		}

		return res;
	}

	Bounds compute_bounds(std::vector<Vertex>& vertices);

	glm::mat4x4 compute_perspective_projection_for_vulkan(float fov_y_rad, float aspect_ratio_w_over_h, float z_near, float z_far);
}