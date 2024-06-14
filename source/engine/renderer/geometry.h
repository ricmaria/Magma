#pragma once

#include "mesh.h"
#include <glm/glm.hpp>

namespace Geometry
{
	void create_sphere(std::vector<Vertex>& out_vertices, float radius, glm::vec3 center, uint32_t resolution, glm::vec3 color);
}