#pragma once

#include <memory>
#include <functional>

#include "core/reflectable.h"
#include "core/injectee.h"

namespace EC
{
	class Component: public Injectee, public Reflectable // Injectee must be the first to allow downcast
	{
		friend class Entity;

	public:
		virtual ~Component() {};

		using ParentType = Reflectable;

		std::vector<TypeId> get_types() const override
		{
			static std::vector<TypeId> type_ids = register_type_and_get_types<Component, ParentType>();
			return type_ids;
		}

	protected:
		struct SiblingRequest
		{
			Reflectable::TypeId type_id;
			std::function<void(Component*, Component*)> assign_sibling;
			std::function<void(Component*, Component*)> dismiss_sibling;

			template<typename TComponent, typename TSibling>
			static SiblingRequest make(TSibling* TComponent::* destination_member)
			{
				SiblingRequest res;

				res.type_id = Reflectable::extract_type<TSibling>();

				res.assign_sibling = [destination_member](Component* component, Component* sibling)
					{
						TComponent* component_typed = static_cast<TComponent*>(component);
						TSibling* sibling_typed = static_cast<TSibling*>(sibling);

						component_typed->*destination_member = sibling_typed;
					};

				res.dismiss_sibling = [destination_member](Component* component, Component* sibling)
					{
						TComponent* component_typed = static_cast<TComponent*>(component);
						component_typed->*destination_member = nullptr;
					};

				return res;
			}

			template<typename TComponent, typename TSibling>
			static SiblingRequest make(std::vector<TSibling*> TComponent::* destination_member)
			{
				SiblingRequest res;

				res.type_id = Reflectable::extract_type<TSibling>();

				res.assign_sibling = [destination_member](Component* component, Component* sibling)
					{
						TComponent* component_typed = static_cast<TComponent*>(component);
						TSibling* sibling_typed = static_cast<TSibling*>(sibling);

						(component_typed->*destination_member).push_back(sibling_typed);
					};

				res.dismiss_sibling = [destination_member](Component* component, Component* sibling)
					{
						TComponent* component_typed = static_cast<TComponent*>(component);
						TSibling* sibling_typed = static_cast<TSibling*>(sibling);

						std::vector<TSibling*>& siblings = component_typed->*destination_member;
						auto it = std::find(siblings.begin(), siblings.end(), sibling);
						assert(it != siblings.end());
						siblings.erase(it);
					};

				return res;
			}


		};

		virtual std::vector<SiblingRequest> get_sibling_requests() const
		{
			static std::vector<SiblingRequest> requests;
			return requests;
		}

		template<typename TParent>
		std::vector<SiblingRequest> add_and_get_siblings_requests(std::vector<SiblingRequest> new_requests) const
		{
			const TParent* this_as_parent = static_cast<const TParent*>(this);
			const auto& parent_requests = this_as_parent->TParent::get_sibling_requests();
			std::vector<SiblingRequest> all_requests;
			all_requests.reserve(new_requests.size() + parent_requests.size());
			all_requests.insert(all_requests.end(), new_requests.begin(), new_requests.end());
			all_requests.insert(all_requests.end(), parent_requests.begin(), parent_requests.end());
			return all_requests;
		}

		virtual void update(float delta_time) {};

		virtual void on_being_added() {};

		virtual void on_being_removed() {};

		virtual void on_sibling_component_added(Component* sibling)
		{
			for (auto& sibling_request : get_sibling_requests())
			{
				if (sibling->is_of_type(sibling_request.type_id))
				{
					sibling_request.assign_sibling(this, sibling);
					break;
				}
			}
		}

		virtual void on_sibling_component_removed(Component* sibling)
		{
			for (auto& sibling_request : get_sibling_requests())
			{
				if (sibling->is_of_type(sibling_request.type_id))
				{
					sibling_request.dismiss_sibling(this, sibling);
					break;
				}
			}
		}
	};
};