#pragma once

#include <vector>

#include "reflectable.h"

class ServiceLocator
{
public:
	template<typename TService>
	TService* get_service() const
	{
		Reflectable::TypeId type_id = Reflectable::extract_type<TService>();

		auto it = find_service(type_id);
		
		if (it != _services.end())
		{
			return static_cast<TService*>(it->service);
		}

		return nullptr;
	}
protected:

	struct ServiceInfo
	{
		Reflectable::TypeId type_id;
		void* service;
	};

	auto find_service(Reflectable::TypeId type_id) const
	{
		auto it = std::find_if(_services.begin(), _services.end(),
			[&type_id](auto& service_info)
			{
				return service_info.type_id == type_id;
			});
		return it;
	}

	std::vector<ServiceInfo> _services;
};

class ServiceRegister : public ServiceLocator
{
public:
	template<typename TService>
	void register_service(TService* service)
	{
		Reflectable::TypeId type_id = Reflectable::extract_type<TService>();

		ServiceInfo service_info{ type_id, service };

		using namespace std::placeholders;

		assert(find_service(type_id) == _services.end());

		_services.push_back(service_info);
	}
};

