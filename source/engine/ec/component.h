#pragma once

#include <vector>
#include <cassert>
#include <memory>
#include <functional>

namespace EC
{
	class Component
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
					sibling_request.assign_sibling(nullptr);
					break;
				}
			}
		};

		template<typename TType>
		bool is_of_type() const
		{
			TypeId type_id = extract_type<TType>();

			return is_of_type(type_id);
		}
	
	protected:
		Component() { register_my_type<decltype(*this)>(); }

		using TypeId = const char*;

		template<typename TType>
		static TypeId extract_type()
		{
			return typeid(TType).name();
		}

		template<typename TType>
		void register_my_type()
		{
			my_type_ids.push_back(extract_type<TType>());
		}

		bool is_of_type(const TypeId& type_id) const
		{
			assert(my_type_ids.size() > 0);

			for (int32_t i = my_type_ids.size() - 1; i >= 0; i--)
			{
				if (my_type_ids[i] == type_id)
				{
					return true;
				}
			}

			return false;
		}

		template<typename TType>
		void request_sibling(TType** destination)
		{
			TypeId type_id = extract_type<TType>();
			auto assign_sibling = [destination](Component* sibling)
				{
					*destination = static_cast<TType*>(sibling);
				};

			_sibling_requests.push_back({ type_id , assign_sibling });
		}

		template<typename TType>
		void request_siblings(std::vector<TType*>& destination)
		{
			TypeId type_id = extract_type<TType>();
			auto assign_sibling = [&destination](Component* sibling)
				{
					destination.push_back(static_cast<TType*>(sibling));
				};

			_sibling_requests.push_back({ type_id , assign_sibling });
		}

	private:
		std::vector<TypeId> my_type_ids;

		struct SiblingRequest
		{
			TypeId type_id;
			std::function<void(Component*)> assign_sibling;
		};

		std::vector<SiblingRequest> _sibling_requests;
	};
};