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

void test_delegates()
{
	auto* delegate_test_void = new DelegateFunction(test_void);
	(*delegate_test_void)();

	auto* delegate_test_int = new DelegateFunction(test_int);
	(*delegate_test_int)();

	auto* delegate_test_void_int = new DelegateFunction(test_void_int);
	(*delegate_test_void_int)(0);

	auto* delegate_test_int_int = new DelegateFunction(test_int_int);
	(*delegate_test_int_int)(0);

	struct TestObject
	{
		void method_void()
		{
			SDL_Log("test method void");
		}

		void method_void_int(int param1)
		{
			SDL_Log("test method void int");
		}

		int method_int()
		{
			SDL_Log("test method int");
			return 0;
		}

		int method_int_int(int param1)
		{
			SDL_Log("test method int int");
			return 0;
		}
	};

	TestObject test_object;

	auto* delegate_method_void = new DelegateMethod(&test_object, &TestObject::method_void);
	(*delegate_method_void)();

	auto* delegate_method_void_int = new DelegateMethod(&test_object, &TestObject::method_void_int);
	(*delegate_method_void_int)(0);

	auto* delegate_method_int = new DelegateMethod(&test_object, &TestObject::method_int);
	(*delegate_method_int)();

	auto* delegate_method_int_int = new DelegateMethod(&test_object, &TestObject::method_int_int);
	(*delegate_method_int_int)(0);
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

	test_delegates();

	return 0;
}
