#pragma once

#include <vector>
#include <cassert>
#include <memory>

namespace EC
{
	class Component
	{
	public:
		virtual ~Component() {};
		virtual void update(float delta_time) {};

		virtual void on_being_added(const std::vector<std::unique_ptr<Component>>& siblings)
		{
			for (auto& sibling_request : _sibling_requests)
			{
				for (auto& sibling : siblings)
				{
					if (sibling->is_of_type(sibling_request.type_id))
					{
						(*sibling_request.destination) = sibling.get();
						break;
					}
				}
			}			
		};

		virtual void on_being_removed() {};

		virtual void on_sibling_component_added(Component* sibling)
		{
			for (auto& sibling_request : _sibling_requests)
			{
				if (sibling->is_of_type(sibling_request.type_id))
				{
					(*sibling_request.destination) = sibling;
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
					(*sibling_request.destination) = nullptr;
					break;
				}
			}
		};

		template<typename TypeArg>
		bool is_of_type() const
		{
			TypeId type_id = extract_type<TypeArg>();

			return is_of_type(type_id);
		}
	
	protected:
		Component() { register_my_type<decltype(*this)>(); }

		using TypeId = const char*;

		template<typename TypeArg>
		static TypeId extract_type()
		{
			return typeid(TypeArg).name();
		}

		template<typename TypeArg>
		void register_my_type()
		{
			my_type_ids.push_back(extract_type<TypeArg>());
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

		template<typename TypeArg>
		void request_sibling(void** destination)
		{
			TypeId type_id = extract_type<TypeArg>();
			_sibling_requests.push_back({ type_id , destination });
		}

	private:
		std::vector<TypeId> my_type_ids;

		struct SiblingRequest
		{
			TypeId type_id;
			void** destination;
		};

		std::vector<SiblingRequest> _sibling_requests;
	};
};