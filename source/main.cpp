#include <engine.h>

#include <ec/mouse_keyboard_input_component.h>
#include <ec/transform_component.h>
#include <ec/camera_component.h>
#include <ec/first_person_controller_component.h>

#include <core/delegate.h>

#include <iostream>

#include "core/injectee.h"

int main(int argc, char* argv[])
{
	MagmaEngine engine;

	engine.init(MagmaEngine::Features::EC);
	
	EC::EntityManager* entity_manager = engine.get_ec_entity_manager();

	EC::Entity* entity = entity_manager->create_entity();

	entity->add_component<EC::MouseKeyboardInputComponent>();
	auto transform_component = entity->add_component<EC::TransformComponent>();
	entity->add_component<EC::FirstPersonControllerComponent>();
	entity->add_component<EC::CameraComponent>();

	transform_component->set_position({ 0.f,-6.f,-10.f });

	engine.run();

	engine.cleanup();

	return 0;
}
