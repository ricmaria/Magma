#include "geometry.h"

#include <functional>

void Geometry::create_sphere(std::vector<Vertex>& out_vertices, float radius, glm::vec3 center, uint32_t resolution, glm::vec3 color)
{
	assert(resolution > 1);

	std::vector<Vertex> unique_vertices;
	unique_vertices.reserve((resolution + 1) * (resolution + 1));

	const float delta = 2.0f / resolution;

	float z = 1.0f;

	auto create_face = [&unique_vertices, delta, resolution, color, radius](std::function<glm::vec3(float, float)> get_position)
		{
			for (uint32_t j = 0; j < resolution + 1; j++)
			{
				for (uint32_t i = 0; i < resolution + 1; i++)
				{
					float a = -1.0f + i * delta;
					float b = -1.0f + j * delta;

					glm::vec3 cube_position = get_position(a,b);

					Vertex vertex;
					vertex.color = color;
					vertex.normal = glm::normalize(cube_position);;
					vertex.position = radius * vertex.normal;
					vertex.uv = { float(i) / float(resolution), float(j) / float(resolution) };

					unique_vertices.push_back(vertex);
				}
			}
		};

	auto add_positive_face = [&out_vertices, &unique_vertices, resolution]()
		{
			for (uint32_t j = 0; j < resolution; j++)
			{
				for (uint32_t i = 0; i < resolution; i++)
				{
					uint32_t index_0_0 = j * (resolution + 1) + i;
					uint32_t index_1_0 = j * (resolution + 1) + i + 1;
					uint32_t index_0_1 = (j + 1) * (resolution + 1) + i;
					uint32_t index_1_1 = (j + 1) * (resolution + 1) + i + 1;

					out_vertices.push_back(unique_vertices[index_0_0]);
					out_vertices.push_back(unique_vertices[index_1_0]);
					out_vertices.push_back(unique_vertices[index_1_1]);
					out_vertices.push_back(unique_vertices[index_0_0]);
					out_vertices.push_back(unique_vertices[index_1_1]);
					out_vertices.push_back(unique_vertices[index_0_1]);
				}
			}
		};

	auto add_negative_face = [&out_vertices, &unique_vertices, resolution]()
		{
			for (uint32_t j = 0; j < resolution; j++)
			{
				for (uint32_t i = 0; i < resolution; i++)
				{
					uint32_t index_0_0 = j * (resolution + 1) + i;
					uint32_t index_1_0 = j * (resolution + 1) + i + 1;
					uint32_t index_0_1 = (j + 1) * (resolution + 1) + i;
					uint32_t index_1_1 = (j + 1) * (resolution + 1) + i + 1;

					out_vertices.push_back(unique_vertices[index_0_0]);
					out_vertices.push_back(unique_vertices[index_1_1]);
					out_vertices.push_back(unique_vertices[index_1_0]);
					out_vertices.push_back(unique_vertices[index_0_0]);
					out_vertices.push_back(unique_vertices[index_0_1]);
					out_vertices.push_back(unique_vertices[index_1_1]);
				}
			}
		};

	// XY

	create_face([](float a, float b)
		{
			return glm::vec3{ a, b, 1.0f };
		});
	add_positive_face();

	unique_vertices.clear();

	create_face([](float a, float b)
		{
			return glm::vec3{ a, b, -1.0f };
		});
	add_negative_face();

	unique_vertices.clear();

	// XZ

	create_face([](float a, float b)
		{
			return glm::vec3{ a, 1.0, b };
		});
	add_positive_face();

	unique_vertices.clear();

	create_face([](float a, float b)
		{
			return glm::vec3{ a, -1.0f, b };
		});
	add_negative_face();

	unique_vertices.clear();

	// YZ

	create_face([](float a, float b)
		{
			return glm::vec3{ 1.0f, a, b };
		});
	add_positive_face();

	unique_vertices.clear();

	create_face([](float a, float b)
		{
			return glm::vec3{ -1.0f, a, b };
		});
	add_negative_face();

	unique_vertices.clear();
}