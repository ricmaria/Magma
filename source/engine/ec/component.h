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

		virtual void on_being_added(const std::vector<std::unique_ptr<Component>>& siblings) {};
		virtual void on_being_removed() {};
		virtual void on_sibling_component_added(const Component& sibling) {};
		virtual void on_sibling_component_removed(const Component& sibling) {};

		template<typename TypeArg>
		bool is_of_type() const
		{
			TypeId type_id = extract_type<TypeArg>();

			assert(my_type_ids.size() > 0);

			for (size_t i = my_type_ids.size() - 1; i >= 0; i--)
			{
				if (my_type_ids[i] == type_id)
				{
					return true;
				}
			}

			return false;
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

	private:
		std::vector<TypeId> my_type_ids;
	};
};