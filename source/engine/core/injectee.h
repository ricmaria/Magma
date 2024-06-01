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