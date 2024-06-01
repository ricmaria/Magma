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

			auto it = _type_id_to_injected.find(type_id);

			if (it != _type_id_to_injected.end())
			{
				void* injected = it->second;

				dependency.inject(&injectee, injected);
			}			
		}
	}

	// If the injected is both Injectee (first) and Refectable, then a derived type must be passed as template argument
	template<typename TInjected = Reflectable>
	static void inject_one(Injectee& injectee, TInjected* injected)
	{
		auto& dependencies = injectee.get_dependencies();

		auto injected_types = injected->get_types();

		for (auto& dependency : dependencies)
		{
			for (auto& injected_type : injected_types)
			{
				if (dependency.type_id == injected_type)
				{
					dependency.inject(&injectee, injected);
				}
			}
		}
	}

	void eject(Injectee& injectee)
	{
		auto& dependencies = injectee.get_dependencies();

		for (auto& dependency : dependencies)
		{
			Reflectable::TypeId type_id = dependency.type_id;

			auto it = _type_id_to_injected.find(type_id);

			if (it != _type_id_to_injected.end())
			{
				void* injected = it->second;

				dependency.eject(&injectee, injected);
			}
		}
	}

	// If the injected is both Injectee (first) and Refectable, then a derived type must be passed as template argument
	template<typename TInjected = Reflectable>
	static void eject_one(Injectee& injectee, TInjected* injected)
	{
		auto& dependencies = injectee.get_dependencies();

		auto injected_types = injected->get_types();

		for (auto& dependency : dependencies)
		{
			Reflectable::TypeId type_id = dependency.type_id;

			auto it = std::find(injected_types.begin(), injected_types.end(), type_id);

			if (it != injected_types.end())
			{
				dependency.eject(&injectee, injected);
			}
		}
	}

	static void eject_all(Injectee& injectee)
	{
		auto& dependencies = injectee.get_dependencies();

		for (auto& dependency : dependencies)
		{
			dependency.eject_all(&injectee);
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

	void unregister_injected(Reflectable* injected)
	{
		for (auto& type_id : injected->get_types())
		{
			auto it = _type_id_to_injected.find(type_id);

			if (it != _type_id_to_injected.end())
			{
				_type_id_to_injected.erase(it);
			}
		}
	}
};