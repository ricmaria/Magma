#include <engine.h>

#include <ec/mouse_keyboard_input_component.h>
#include <ec/transform_component.h>
#include <ec/camera_component.h>
#include <ec/first_person_controller_component.h>
#include <ec/mesh_render_component.h>

#include <core/delegate.h>

#include <iostream>

#include "core/injectee.h"

int main(int argc, char* argv[])
{
	MagmaEngine engine;

	engine.init(MagmaEngine::Features::EC);
	
	EC::EntityManager* entity_manager = engine.get_ec_entity_manager();

	EC::Entity* player_entity = entity_manager->create_entity();

	player_entity->add_component<EC::MouseKeyboardInputComponent>();
	auto transform_component = player_entity->add_component<EC::TransformComponent>();
	player_entity->add_component<EC::FirstPersonControllerComponent>();
	player_entity->add_component<EC::CameraComponent>();

	transform_component->set_position({ 0.f,-6.f,-10.f });

	EC::Entity* monkey_entity = entity_manager->create_entity();

	auto monkey_mesh_component = EC::Entity::create_component<EC::MeshRenderComponent>();
	monkey_mesh_component->set_mesh_name("monkey");
	monkey_mesh_component->set_material_name("defaultmesh");
	monkey_mesh_component->_temp_transform = glm::mat4{ 1.0f };

	monkey_entity->add_component(std::move(monkey_mesh_component));

	EC::Entity* empire_entity = entity_manager->create_entity();

	auto empire_mesh_component = EC::Entity::create_component<EC::MeshRenderComponent>();
	empire_mesh_component->set_mesh_name("empire");
	empire_mesh_component->set_material_name("texturedmesh");
	empire_mesh_component->_temp_transform = glm::translate(glm::vec3{ 5,-10,0 });

	empire_entity->add_component(std::move(empire_mesh_component));

	EC::Entity* triangles_entity = entity_manager->create_entity();

	for (int x = -20; x <= 20; x++)
	{
		for (int y = -20; y <= 20; y++)
		{
			auto triangle_mesh_component = EC::Entity::create_component<EC::MeshRenderComponent>();
			triangle_mesh_component->set_mesh_name("triangle");
			triangle_mesh_component->set_material_name("defaultmesh");
			glm::mat4 translation = glm::translate(glm::mat4{ 1.0 }, glm::vec3(x, 0, y));
			glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2, 0.2, 0.2));
			triangle_mesh_component->_temp_transform = translation * scale;

			triangles_entity->add_component(std::move(triangle_mesh_component));
		}
	}

	engine.run();

	engine.cleanup();

	return 0;
}
