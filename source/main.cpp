#include <engine.h>

#include <ec/keyboard_input_component.h>
#include <ec/camera_component.h>
#include <ec/entity_manager.h>

#include <core/delegate.h>

#include <iostream>

#include <SDL.h>

void test()
{
	SDL_Log("test void");
}

int test_int()
{
	SDL_Log("test int");

	return 0;
}

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

	auto* delegate_test = new DelegateFunctionNoParams(test);
	(*delegate_test)();

	auto* delegate_test_int = new DelegateFunctionNoParams(test_int);
	(*delegate_test_int)();

	return 0;
}
