#include <engine.h>

#include <ec/keyboard_input_component.h>
#include <ec/camera_component.h>
#include <ec/entity_manager.h>

#include <core/delegate.h>

#include <iostream>

#include <SDL.h>

void test_void()
{
	SDL_Log("test void");
}

int test_int()
{
	SDL_Log("test int");

	return 0;
}

void test_void_int(int value)
{
	SDL_Log("test void int");
}

int test_int_int(int value)
{
	SDL_Log("test int int");

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

	auto* delegate_test_void = new DelegateFunctionNoParams(test_void);
	(*delegate_test_void)();

	auto* delegate_test_int = new DelegateFunctionNoParams(test_int);
	(*delegate_test_int)();

	auto* delegate_test_void_int = new DelegateFunctionOneParam(test_void_int);
	(*delegate_test_void_int)(0);

	auto* delegate_test_int_int = new DelegateFunctionOneParam(test_int_int);
	(*delegate_test_int_int)(0);

	return 0;
}
