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

			auto it = m_type_id_to_injected.find(type_id);

			if (it != m_type_id_to_injected.end())
			{
				void* injected = it->second;

				dependency.inject(&injectee, injected);
			}			
		}
	}

	// If the injected is both Injectee (first) and Reflectable, then a derived type must be passed as template argument
	template<typename TInjected>
	static void inject_all_types_of_injected(Injectee& injectee, TInjected* injected)
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

	template<typename TInjected>
	static void inject_just_type_of_injected(Injectee& injectee, TInjected* injected)
	{
		static Reflectable::TypeId injected_type_id = Reflectable::extract_type<TInjected>();

		auto& dependencies = injectee.get_dependencies();

		for (auto& dependency : dependencies)
		{
			if (dependency.type_id == injected_type_id)
			{
				dependency.inject(&injectee, injected);
			}
		}
	}

	void eject(Injectee& injectee)
	{
		auto& dependencies = injectee.get_dependencies();

		for (auto& dependency : dependencies)
		{
			Reflectable::TypeId type_id = dependency.type_id;

			auto it = m_type_id_to_injected.find(type_id);

			if (it != m_type_id_to_injected.end())
			{
				void* injected = it->second;

				dependency.eject(&injectee, injected);
			}
		}
	}

	// If the injected is both Injectee (first) and Refectable, then a derived type must be passed as template argument
	template<typename TInjected>
	static void eject_all_types_of_injected(Injectee& injectee, TInjected* injected)
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

	template<typename TInjected>
	static void eject_just_type_of_injected(Injectee& injectee)
	{
		static Reflectable::TypeId injected_type_id = Reflectable::extract_type<TInjected>();

		auto& dependencies = injectee.get_dependencies();

		for (auto& dependency : dependencies)
		{
			if (dependency.type_id == injected_type_id)
			{
				dependency.eject(&injectee, nullptr);
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

	std::unordered_map<Reflectable::TypeId, void*> m_type_id_to_injected;
};

class InjectorRegister : public Injector
{
public:
	void register_all_types(Reflectable* injected)
	{
		static Reflectable::TypeId reflectable_type_id = Reflectable::extract_type<Reflectable>();
		
		for (auto& type_id : injected->get_types())
		{
			if (type_id != reflectable_type_id && !m_type_id_to_injected.contains(type_id))
			{
				m_type_id_to_injected[type_id] = injected;
			}
		}
	}

	template<typename TInjected>
	void register_one_type(TInjected* injected)
	{
		static Reflectable::TypeId reflectable_type_id = Reflectable::extract_type<Reflectable>();
		static Reflectable::TypeId injected_type_id = Reflectable::extract_type<TInjected>();

		if (injected_type_id != reflectable_type_id)
		{
			m_type_id_to_injected[injected_type_id] = injected;
		}
	}

	void unregister_all_types(Reflectable* injected)
	{
		for (auto& type_id : injected->get_types())
		{
			auto it = m_type_id_to_injected.find(type_id);

			if (it != m_type_id_to_injected.end())
			{
				m_type_id_to_injected.erase(it);
			}
		}
	}

	template<typename TInjected>
	void unregister_one_type()
	{
		static Reflectable::TypeId injected_type_id = Reflectable::extract_type<TInjected>();

		auto it = m_type_id_to_injected.find(injected_type_id);

		if (it != m_type_id_to_injected.end())
		{
			m_type_id_to_injected.erase(it);
		}
	}

	void unregister_injected(void* injected)
	{
		std::erase_if(m_type_id_to_injected, [injected](const auto& pair)
			{ return pair.second == injected; });
	}
};