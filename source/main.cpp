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

	auto keyboard_input_component = entity->add_component<EC::KeyboardInputComponent>();
	
	auto camera_component = entity->add_component<EC::CameraComponent>();

	entity->remove_component<EC::KeyboardInputComponent>();

	return 0;
}
