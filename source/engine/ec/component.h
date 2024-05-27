#pragma once

#include <memory>
#include <functional>

#include "core/reflectable.h"

namespace EC
{
	class Component: protected Reflectable
	{
	public:
		virtual ~Component() {};
		virtual void update(float delta_time) {};

		virtual void on_being_added() {};

		virtual void on_being_removed() {};

		virtual void on_sibling_component_added(Component* sibling)
		{
			for (auto& sibling_request : _sibling_requests)
			{
				if (sibling->is_of_type(sibling_request.type_id))
				{
					sibling_request.assign_sibling(sibling);
					break;
				}
			}
		}

		virtual void on_sibling_component_removed(Component* sibling)
		{
			for (auto& sibling_request : _sibling_requests)
			{
				if (sibling->is_of_type(sibling_request.type_id))
				{
					sibling_request.dismiss_sibling(sibling);
					break;
				}
			}
		}
	
	protected:
		Component() { register_my_type<decltype(*this)>(); }

		template<typename TType>
		void register_sibling_request(TType** destination)
		{
			TypeId type_id = extract_type<TType>();
			auto assign_sibling = [destination](Component* sibling)
				{
					*destination = static_cast<TType*>(sibling);
				};
			auto dismiss_sibling = [destination](Component* sibling)
				{
					assert(*destination == static_cast<TType*>(sibling));
					*destination = nullptr;
				};

			_sibling_requests.push_back({ type_id , assign_sibling, dismiss_sibling });
		}

		template<typename TType>
		void register_siblings_request(std::vector<TType*>& destination)
		{
			TypeId type_id = extract_type<TType>();
			auto assign_sibling = [&destination](Component* sibling)
				{
					destination.push_back(static_cast<TType*>(sibling));
				};
			auto dismiss_sibling = [&destination](Component* sibling)
				{
					auto it = std::find(destination.begin(), destination.end(), sibling);
					assert(it != destination.end());
					destination.erase(it);
				};

			_sibling_requests.push_back({ type_id , assign_sibling, dismiss_sibling });
		}

	private:

		struct SiblingRequest
		{
			Reflectable::TypeId type_id;
			std::function<void(Component*)> assign_sibling;
			std::function<void(Component*)> dismiss_sibling;
		};

		std::vector<SiblingRequest> _sibling_requests;
	};
};