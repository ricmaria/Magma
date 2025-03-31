#include "geometry.h"

#include <functional>

#include <glm/gtc/matrix_transform.hpp>


void Geometry::create_sphere(std::vector<Vertex>& out_vertices, float radius, glm::vec3 center, uint32_t resolution, glm::vec4 color)
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

					glm::vec3 cube_position = get_position(a, b);

					Vertex vertex;
					vertex.color = color;
					vertex.normal = glm::normalize(cube_position);
					vertex.position = radius * vertex.normal;
					vertex.uv_x = float(i) / float(resolution);
					vertex.uv_y = float(j) / float(resolution);

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

void Geometry::create_piramyd(std::vector<Vertex>& out_vertices, std::vector<uint32_t>& out_indices, float base, float height, glm::vec4 color)
{
	const float sqert2div2 = std::sqrt(2.0f) / 2.0f;
	const float half_base = base * 0.5f;
	const float one_third_base = base / 3.0f;
	const float one_third_height = height / 3.0f;

	const uint32_t a = out_vertices.size() + 0;
	const uint32_t b = out_vertices.size() + 1;
	const uint32_t c = out_vertices.size() + 2;
	const uint32_t d = out_vertices.size() + 3;
	const uint32_t e = out_vertices.size() + 4;
	
	Vertex vertex;
	vertex.color = color;
	vertex.position = { half_base, 0.0f, half_base };
	vertex.normal = { one_third_base, one_third_height, one_third_base };
	vertex.uv_x = 1.0f;
	vertex.uv_y = 0.0f;
	out_vertices.push_back(vertex);

	vertex.position = { half_base, 0.0f, -half_base };
	vertex.normal = { one_third_base, one_third_height, -one_third_base };
	vertex.uv_x = 1.0f;
	vertex.uv_y = 1.0f;
	out_vertices.push_back(vertex);

	vertex.position = { -half_base, 0.0f, -half_base };
	vertex.normal = { -one_third_base, one_third_height, -one_third_base };
	vertex.uv_x = 0.0f;
	vertex.uv_y = 1.0f;
	out_vertices.push_back(vertex);

	vertex.position = { -half_base, 0.0f, half_base };
	vertex.normal = { -one_third_base, -one_third_height, one_third_base };
	vertex.uv_x = 0.0f;
	vertex.uv_y = 0.0f;
	out_vertices.push_back(vertex);

	vertex.position = { 0.0f, height, 0.0f };
	vertex.normal = { 0.0f, 1.0f, 0.0f };	
	vertex.uv_x = 0.5f;
	vertex.uv_y = 0.5f;
	out_vertices.push_back(vertex);

	out_indices.push_back(c);
	out_indices.push_back(b);
	out_indices.push_back(a);

	out_indices.push_back(a);
	out_indices.push_back(d);
	out_indices.push_back(c);

	out_indices.push_back(e);
	out_indices.push_back(a);
	out_indices.push_back(b);

	out_indices.push_back(e);
	out_indices.push_back(b);
	out_indices.push_back(c);

	out_indices.push_back(e);
	out_indices.push_back(c);
	out_indices.push_back(d);

	out_indices.push_back(e);
	out_indices.push_back(d);
	out_indices.push_back(a);
}


void Geometry::create_piramyd(std::vector<Vertex>& out_vertices, float base, float height, glm::vec4 color)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	create_piramyd(vertices, indices, base, height, color);

	std::vector<Vertex> unique_vertices = convert_to_unique_vertices(vertices, indices);

	out_vertices.insert(
		out_vertices.end(),
		std::make_move_iterator(unique_vertices.begin()),
		std::make_move_iterator(unique_vertices.end())
	);
}

void Geometry::create_box(std::vector<Vertex>& out_vertices, std::vector<uint32_t>& out_indices, glm::vec3 dimensions, glm::vec4 color)
{
	const uint32_t a = out_vertices.size() + 0;
	const uint32_t b = out_vertices.size() + 1;
	const uint32_t c = out_vertices.size() + 2;
	const uint32_t d = out_vertices.size() + 3;
	const uint32_t e = out_vertices.size() + 4;
	const uint32_t f = out_vertices.size() + 5;
	const uint32_t g = out_vertices.size() + 6;
	const uint32_t h = out_vertices.size() + 7;

	const glm::vec3 half_dim = dimensions / 2.0f;

	Vertex vertex;
	vertex.color = color;

	vertex.position = { half_dim.x, half_dim.y, half_dim.z };
	vertex.normal = glm::normalize(vertex.position);
	vertex.uv_x = 0.0f;
	vertex.uv_y = 0.0f;
	out_vertices.push_back(vertex);

	vertex.position = { half_dim.x, half_dim.y, -half_dim.z };
	vertex.normal = glm::normalize(vertex.position);
	vertex.uv_x = 0.0f;
	vertex.uv_y = 1.0f;
	out_vertices.push_back(vertex);

	vertex.position = { -half_dim.x, half_dim.y, -half_dim.z };
	vertex.normal = glm::normalize(vertex.position);
	vertex.uv_x = 1.0f;
	vertex.uv_y = 1.0f;
	out_vertices.push_back(vertex);

	vertex.position = { -half_dim.x, half_dim.y, half_dim.z };
	vertex.normal = glm::normalize(vertex.position);
	vertex.uv_x = 1.0f;
	vertex.uv_y = 0.0f;
	out_vertices.push_back(vertex);


	vertex.position = { half_dim.x, -half_dim.y, half_dim.z };
	vertex.normal = glm::normalize(vertex.position);
	vertex.uv_x = 0.0f;
	vertex.uv_y = 0.0f;
	out_vertices.push_back(vertex);

	vertex.position = { half_dim.x, -half_dim.y, -half_dim.z };
	vertex.normal = glm::normalize(vertex.position);
	vertex.uv_x = 0.0f;
	vertex.uv_y = 1.0f;
	out_vertices.push_back(vertex);

	vertex.position = { -half_dim.x, -half_dim.y, -half_dim.z };
	vertex.normal = glm::normalize(vertex.position);
	vertex.uv_x = 1.0f;
	vertex.uv_y = 1.0f;
	out_vertices.push_back(vertex);

	vertex.position = { -half_dim.x, -half_dim.y, half_dim.z };
	vertex.normal = glm::normalize(vertex.position);
	vertex.uv_x = 1.0f;
	vertex.uv_y = 0.0f;
	out_vertices.push_back(vertex);

	out_indices.push_back(a);
	out_indices.push_back(b);
	out_indices.push_back(c);

	out_indices.push_back(c);
	out_indices.push_back(d);
	out_indices.push_back(a);

	out_indices.push_back(e);
	out_indices.push_back(h);
	out_indices.push_back(g);

	out_indices.push_back(g);
	out_indices.push_back(f);
	out_indices.push_back(e);

	out_indices.push_back(a);
	out_indices.push_back(e);
	out_indices.push_back(f);

	out_indices.push_back(f);
	out_indices.push_back(b);
	out_indices.push_back(a);

	out_indices.push_back(c);
	out_indices.push_back(b);
	out_indices.push_back(f);

	out_indices.push_back(f);
	out_indices.push_back(g);
	out_indices.push_back(c);

	out_indices.push_back(c);
	out_indices.push_back(g);
	out_indices.push_back(h);

	out_indices.push_back(h);
	out_indices.push_back(d);
	out_indices.push_back(c);

	out_indices.push_back(d);
	out_indices.push_back(h);
	out_indices.push_back(e);

	out_indices.push_back(e);
	out_indices.push_back(a);
	out_indices.push_back(d);
}

void Geometry::create_box(std::vector<Vertex>& out_vertices, glm::vec3 dimensions, glm::vec4 color)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	create_box(vertices, indices, dimensions, color);

	std::vector<Vertex> unique_vertices = convert_to_unique_vertices(vertices, indices);

	out_vertices.insert(
		out_vertices.end(),
		std::make_move_iterator(unique_vertices.begin()),
		std::make_move_iterator(unique_vertices.end())
	);
}

void Geometry::create_arrow(std::vector<Vertex>& out_vertices, std::vector<uint32_t>& out_indices, glm::vec4 color)
{
	uint32_t box_start_index = out_vertices.size();

	create_box(out_vertices, out_indices, { 0.25f, 1.0f, 0.25f }, color);

	glm::vec3 box_translation{ 0.0f, 0.5f, 0.0f };

	for (uint32_t i = box_start_index; i < out_vertices.size(); ++i)
	{
		Vertex& vertex = out_vertices[i];
		vertex.position += box_translation;
	}

	const uint32_t pyramid_start_index = out_vertices.size();

	create_piramyd(out_vertices, out_indices, 0.5f, 0.25, color);

	glm::vec3 pyramid_translation{ 0.0f, 1.0f, 0.0f };

	for (uint32_t i = pyramid_start_index; i < out_vertices.size(); ++i)
	{
		Vertex& vertex = out_vertices[i];
		vertex.position += pyramid_translation;
	}
}

void Geometry::create_arrow(std::vector<Vertex>&out_vertices, glm::vec4 color)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	create_arrow(vertices, indices, color);

	std::vector<Vertex> unique_vertices = convert_to_unique_vertices(vertices, indices);

	out_vertices.insert(
		out_vertices.end(),
		std::make_move_iterator(unique_vertices.begin()),
		std::make_move_iterator(unique_vertices.end())
	);
}

void Geometry::create_gizmo(std::vector<Vertex>& out_vertices, std::vector<uint32_t>& out_indices)
{
	uint32_t x_arrow_start_index = out_vertices.size();

	create_arrow(out_vertices, out_indices, { 1.0f, 0.0f, 0.0f, 1.0f });

	glm::mat4 rotate_x = glm::rotate(glm::mat4{ 1.0f }, glm::half_pi<float>(), glm::vec3{ 0.0f, 0.0f, -1.0f });

	for (uint32_t i = x_arrow_start_index; i < out_vertices.size(); ++i)
	{
		Vertex& vertex = out_vertices[i];
		glm::vec4 vertex_4 = { vertex.position, 1.0f };
		vertex_4 = rotate_x * vertex_4;
		vertex.position = vertex_4;
		glm::vec4 normal_4 = { vertex.normal, 1.0f };
		normal_4 = rotate_x * normal_4;
		vertex.normal = normal_4;
	}

	uint32_t y_arrow_start_index = out_vertices.size();
	
	create_arrow(out_vertices, out_indices, { 0.0f, 1.0f, 0.0f, 1.0f });

	uint32_t z_arrow_start_index = out_vertices.size();

	create_arrow(out_vertices, out_indices, { 0.0f, 0.0f, 1.0f, 1.0f });

	glm::mat4 rotate_z = glm::rotate(glm::mat4{ 1.0f }, glm::half_pi<float>(), glm::vec3{ 1.0f, 0.0f, 0.0f });

	for (uint32_t i = z_arrow_start_index; i < out_vertices.size(); ++i)
	{
		Vertex& vertex = out_vertices[i];
		glm::vec4 vertex_4 = { vertex.position, 1.0f };
		vertex_4 = rotate_z * vertex_4;
		vertex.position = vertex_4;
		glm::vec4 normal_4 = { vertex.normal, 1.0f };
		normal_4 = rotate_z * normal_4;
		vertex.normal = normal_4;
	}
}

void Geometry::create_gizmo(std::vector<Vertex>& out_vertices)
{
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	create_gizmo(vertices, indices);

	std::vector<Vertex> unique_vertices = convert_to_unique_vertices(vertices, indices);

	out_vertices.insert(
		out_vertices.end(),
		std::make_move_iterator(unique_vertices.begin()),
		std::make_move_iterator(unique_vertices.end())
	);
}

Bounds Geometry::compute_bounds(std::vector<Vertex>& vertices)
{
	Bounds bounds;

	glm::vec3 minpos = vertices[0].position;
	glm::vec3 maxpos = vertices[0].position;

	for (size_t i = 0; i < vertices.size(); i++)
	{
		minpos = glm::min(minpos, vertices[i].position);
		maxpos = glm::max(maxpos, vertices[i].position);
	}
	// calculate origin and extents from the min/max, use extent lenght for radius
	bounds.origin = (maxpos + minpos) / 2.f;
	bounds.extents = (maxpos - minpos) / 2.f;
	bounds.sphereRadius = glm::length(bounds.extents);

	return bounds;
}

glm::mat4x4 Geometry::compute_perspective_projection_for_vulkan(float fov_y_rad, float aspect_ratio_w_over_h, float z_near, float z_far)
{
	// formula taken from https://johannesugb.github.io/gpu-programming/setting-up-a-proper-vulkan-projection-matrix/

	glm::mat4x4 project(0.0f);

	project[0][0] = (1.0f / aspect_ratio_w_over_h) / std::tanf(fov_y_rad / 2.0f);	// first index is column, second is row
	project[1][1] = 1.0f / std::tanf(fov_y_rad / 2.0f);
	project[2][2] = z_far / (z_far - z_near);
	project[2][3] = 1.0f;
	project[3][2] = -(z_near * z_far) / (z_far - z_near);

	glm::mat4x4 vulkan_axes_align(1.0f);
	vulkan_axes_align[1][1] = -1.0f;
	vulkan_axes_align[2][2] = -1.0f;

	glm::mat4x4 res = project * vulkan_axes_align;

	return res;
}