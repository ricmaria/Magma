#pragma once

#include "reflectable.h"

#include <functional>

class Injectee
{
    friend class Injector;

protected:
    Injectee() {}

    struct Dependency
    {
        Reflectable::TypeId type_id;
        std::function<void(void*, void*)> inject;
        std::function<void(void*, void*)> eject;
        std::function<void(void*)> eject_all;

        template<typename TInjectee, typename TInjected>
        static Dependency make(TInjected* TInjectee::*injectee_member)
        {
            Dependency res;

            res.type_id = Reflectable::extract_type<TInjected>();

            res.inject = [injectee_member](void* injectee, void* injected)
                {
                    TInjectee* injectee_typed = static_cast<TInjectee*>(injectee);
                    TInjected* injected_typed = static_cast<TInjected*>(injected);
                    
                    injectee_typed->*injectee_member = injected_typed;
                };

            res.eject = [injectee_member](void* injectee, void* injected)
                {
                    TInjectee* injectee_typed = static_cast<TInjectee*>(injectee);

                    injectee_typed->*injectee_member = nullptr;
                };

            res.eject_all = [injectee_member](void* injectee)
                {
                    TInjectee* injectee_typed = static_cast<TInjectee*>(injectee);

                    injectee_typed->*injectee_member = nullptr;
                };

            return res;
        }

        template<typename TInjectee, typename TInjected>
        static Dependency make(std::vector<TInjected*> TInjectee::* injectee_member)
        {
            Dependency res;

            res.type_id = Reflectable::extract_type<TInjected>();

            res.inject = [injectee_member](void* injectee, void* injected)
                {
                    TInjectee* injectee_typed = static_cast<TInjectee*>(injectee);
                    TInjected* injected_typed = static_cast<TInjected*>(injected);

                    (injectee_typed->*injectee_member).push_back(injected_typed);
                };

            res.eject = [injectee_member](void* injectee, void* injected)
                {
                    TInjectee* injectee_typed = static_cast<TInjectee*>(injectee);
                    TInjected* injected_typed = static_cast<TInjected*>(injected);

                    std::vector<TInjected*>& injecteds = injectee_typed->*injectee_member;
                    auto it = std::find(injecteds.begin(), injecteds.end(), injected);
                    assert(it != injecteds.end());
                    injecteds.erase(it);
                };

            res.eject_all = [injectee_member](void* injectee)
                {
                    TInjectee* injectee_typed = static_cast<TInjectee*>(injectee);

                    std::vector<TInjected*>& injecteds = injectee_typed->*injectee_member;
                    injecteds.clear();
                };

            return res;
        }
    };

    virtual const std::vector<Dependency>& get_dependencies() const
    {
        static std::vector<Dependency> dependencies;
        return dependencies;
    }
    
    template<typename TParent>
    inline std::vector<Dependency> register_and_get_dependencies(const std::vector<Dependency>& dependencies) const
    {
        const TParent* this_as_parent = static_cast<const TParent*>(this);
        const auto& parent_dependencies = this_as_parent->TParent::get_dependencies();
        std::vector<Dependency> all_dependencies;
        all_dependencies.reserve(dependencies.size() + parent_dependencies.size());
        all_dependencies.insert(all_dependencies.end(), dependencies.begin(), dependencies.end());
        all_dependencies.insert(all_dependencies.end(), parent_dependencies.begin(), parent_dependencies.end());
        return all_dependencies;
    }
};