#pragma once

#include "injectee.h"
#include "reflectable.h"
#include <unordered_map>

class Injector
{
public:
	void inject(Injectee& injectee)
	{
		auto& dependencies = injectee.get_dependencies();

		for (auto& dependency : dependencies)
		{
			Reflectable::TypeId type_id = dependency.type_id;

			assert(_type_id_to_injected.contains(type_id) && "Missing injected object.");

			void* injected = _type_id_to_injected[type_id];

			dependency.inject(&injectee, injected);
		}
	}

protected:
	struct InjectedInfo
	{
		Reflectable::TypeId type_id;
		Reflectable* injected;
	};

	std::unordered_map<Reflectable::TypeId, void*> _type_id_to_injected;
};

class InjectorRegister : public Injector
{
public:
	void register_injected(Reflectable* injected)
	{
		static Reflectable::TypeId reflectable_type_id = Reflectable::extract_type<Reflectable>();
		
		for (auto& type_id : injected->get_types())
		{
			if (type_id != reflectable_type_id && !_type_id_to_injected.contains(type_id))
			{
				_type_id_to_injected[type_id] = injected;
			}
		}
	}
};