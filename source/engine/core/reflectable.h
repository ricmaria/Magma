#pragma once

#include <vector>
#include <cassert>

class Reflectable
{
public:

    using TypeId = const char*;

    // This must be overridden by every subclass
    virtual std::vector<TypeId> get_types() const
    {
        static std::vector<TypeId> type_ids = register_top_type();
        return type_ids;
    }

    TypeId get_actual_type() const
    {
        std::vector<TypeId> type_ids = get_types();
        assert(type_ids.size() > 0);
        return type_ids[type_ids.size() - 1];
    }

    bool is_of_type(TypeId type_id) const
    {
        std::vector<TypeId> type_ids = get_types();

        for (const auto& stored_type : type_ids)
        {
            if (type_id == stored_type)
            {
                return true;
            }
        }

        return false;
    }

    template<typename TType>
    bool is_of_type()
    {
        TypeId type_id = extract_type<TType>();
        return is_of_type(type_id);
    }

    template<typename TType>
    static TypeId extract_type()
    {
        return typeid(TType).name();
    }

protected:    

    template<typename TSelf, typename TParent>
    std::vector<TypeId> register_type_and_get_types() const
    {
        TypeId type_id = extract_type<TSelf>();
        const TParent* this_as_parent = static_cast<const TParent*>(this);
        std::vector<TypeId> type_ids = this_as_parent->TParent::get_types();
        type_ids.push_back(type_id);
        return type_ids;
    }

private:

    static std::vector<TypeId> register_top_type()
    {
        TypeId type_id = extract_type<Reflectable>();
        std::vector<TypeId> type_ids;
        type_ids.push_back(type_id);
        return type_ids;
    }
};