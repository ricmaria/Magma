#include <engine.h>

#include <ec/keyboard_input_component.h>
#include <ec/transform_component.h>
#include <ec/camera_component.h>
#include <ec/first_person_controller_component.h>

#include <core/delegate.h>

#include <iostream>

#include <SDL.h>

#include "core/injectee.h"

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
	struct TestObject
	{
		void method_void()
		{
			SDL_Log("test class member void");
		}

		void method_void_int(int param1)
		{
			SDL_Log("test class member void int");
		}

		int method_int()
		{
			SDL_Log("test class member int");
			return 0;
		}

		int method_int_int(int param1)
		{
			SDL_Log("test class member int int");
			return 0;
		}
	};

	int a = 0;
	TestObject test_object;

	Delegate<void> alt_void(test_void);
	alt_void();

	Delegate<int> alt_int(test_int);
	a = alt_int();

	Delegate<void, int> alt_void_int(test_void_int);
	alt_void_int(0);

	Delegate<int, int> alt_int_int(test_int_int);
	a = alt_int_int(0);

	Delegate<void> alt_obj_void(&test_object, &TestObject::method_void);
	alt_obj_void();

	Delegate<int> alt_obj_int(&test_object, &TestObject::method_int);
	a = alt_obj_int();

	Delegate<void, int> alt_obj_void_int(&test_object, &TestObject::method_void_int);
	alt_obj_void_int(0);

	Delegate<int, int> alt_obj_int_int(&test_object, &TestObject::method_int_int);
	a = alt_obj_int_int(0);
}

void test_injection(MagmaEngine& engine)
{
	Renderer* renderer = engine.get_renderer();
	EC::EntityManager* entity_manager = engine.get_ec_entity_manager();

	class A : public Injectee
	{
	private:
		EC::EntityManager* _entity_manager;
		Renderer* _renderer;
	public:
		const std::vector<Dependency>& get_dependencies() const override
		{
			static Dependency dependency1 = Dependency::make(&A::_entity_manager);
			static Dependency dependency2 = Dependency::make(&A::_renderer);
			static auto dependencies = register_dependencies<Injectee>(dependency1, dependency2);
			return dependencies;
		}
	};

	A a;

	for (auto& dependency : a.get_dependencies())
	{
		SDL_Log("dependency %s", dependency.get_type_id());

		if (dependency.get_type_id() == Reflectable::extract_type<EC::EntityManager>())
		{
			dependency.inject(&a, entity_manager);
		}

		if (dependency.get_type_id() == Reflectable::extract_type<Renderer>())
		{
			dependency.inject(&a, renderer);
		}
	}
}

int main(int argc, char* argv[])
{
	MagmaEngine engine;

	engine.init(MagmaEngine::Features::EC);
	
	EC::EntityManager* entity_manager = engine.get_ec_entity_manager();

	EC::Entity* entity = entity_manager->create_entity();

	entity->add_component<EC::KeyboardInputComponent>();
	auto transform_component = entity->add_component<EC::TransformComponent>();
	entity->add_component<EC::FirstPersonControllerComponent>();
	entity->add_component<EC::CameraComponent>();

	transform_component->set_position({ 0.f,-6.f,-10.f });

	engine.run();

	// test_delegates();
	// test_injection()

	engine.cleanup();

	return 0;
}
