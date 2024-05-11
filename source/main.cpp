#include <engine.h>

#include <ec/keyboard_input_component.h>
#include <ec/camera_component.h>
#include <ec/entity_manager.h>

int main(int argc, char* argv[])
{
	//MagmaEngine engine;

	//engine.init();	
	//
	//engine.run();	

	//engine.cleanup();

	EC::EntityManager entity_manager;

	EC::Entity* entity = entity_manager.create_entity();

	auto keyboard_input_component = entity_manager.create_component<EC::KeyboardInputComponent>();

	entity->add_component(std::move(keyboard_input_component));
	
	auto camera_component = entity_manager.create_component<EC::CameraComponent>();

	entity->add_component(std::move(camera_component));

	entity->remove_component<EC::KeyboardInputComponent>();

	return 0;
}
