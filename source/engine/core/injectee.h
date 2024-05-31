#pragma once

#include "reflectable.h"

#include <functional>

class Injectee
{
    friend class Injector;

protected:
    Injectee() {}

    class Dependency
    {
    public:
        template<typename TInjectee, typename TInjected>
        static Dependency make(TInjected* TInjectee::*injectee_member)
        {
            Dependency res;

            res._type_id = Reflectable::extract_type<TInjected>();

            res._inject = [injectee_member](void* injectee, void* injected)
                {
                    TInjectee* injectee_typed = static_cast<TInjectee*>(injectee);
                    TInjected* injected_typed = static_cast<TInjected*>(injected);
                    
                    injectee_typed->*injectee_member = injected_typed;
                };

            return res;
        }

        inline Reflectable::TypeId get_type_id() const { return _type_id; }

        void inject(void* injectee, void* injected) const
        {
            _inject(injectee, injected);
        }

    private:
        Reflectable::TypeId _type_id;
        std::function<void(void*, void*)> _inject;
    };

    virtual const std::vector<Dependency>& get_dependencies() const
    {
        static std::vector<Dependency> dependencies;
        return dependencies;
    }

    template<typename TParent>
    inline std::vector<Dependency> register_dependencies(const Dependency& dependency) const
    {
        std::vector<Dependency> dependencies;
        dependencies.push_back(dependency);
        return register_dependencies<TParent>(dependencies);
    }

    template<typename TParent>
    inline std::vector<Dependency> register_dependencies(const Dependency& dependency1, const Dependency& dependency2) const
    {
        std::vector<Dependency> dependencies;
        dependencies.push_back(dependency1);
        dependencies.push_back(dependency2);
        return register_dependencies<TParent>(dependencies);
    }

    template<typename TParent>
    inline std::vector<Dependency> register_dependencies(const Dependency& dependency1, const Dependency& dependency2, const Dependency& dependency3) const
    {
        std::vector<Dependency> dependencies;
        dependencies.push_back(dependency1);
        dependencies.push_back(dependency2);
        dependencies.push_back(dependency3);
        return register_dependencies<TParent>(dependencies);
    }

    template<typename TParent>
    inline std::vector<Dependency> register_dependencies(const Dependency& dependency1, const Dependency& dependency2,
        const Dependency& dependency3, const Dependency& dependency4) const
    {
        std::vector<Dependency> dependencies;
        dependencies.push_back(dependency1);
        dependencies.push_back(dependency2);
        dependencies.push_back(dependency3);
        dependencies.push_back(dependency4);
        return register_dependencies<TParent>(dependencies);
    }

    template<typename TParent>
    inline std::vector<Dependency> register_dependencies(const Dependency& dependency1, const Dependency& dependency2,
        const Dependency& dependency3, const Dependency& dependency4, const Dependency& dependency5) const
    {
        std::vector<Dependency> dependencies;
        dependencies.push_back(dependency1);
        dependencies.push_back(dependency2);
        dependencies.push_back(dependency3);
        dependencies.push_back(dependency4);
        dependencies.push_back(dependency5);
        return register_dependencies<TParent>(dependencies);
    }

private:
    
    template<typename TParent>
    inline std::vector<Dependency> register_dependencies(const std::vector<Dependency>& dependencies) const
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