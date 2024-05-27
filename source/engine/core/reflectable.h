#pragma once

#include <vector>
#include <cassert>

class Reflectable
{
public:
	using TypeId = const char*;

	template<typename TType>
	static TypeId extract_type()
	{
		return typeid(TType).name();
	}

	template<typename TType>
	void register_my_type()
	{
		_my_type_ids.push_back(extract_type<TType>());
	}

	bool is_of_type(const TypeId& type_id) const
	{
		assert(_my_type_ids.size() > 0);

		for (int32_t i = _my_type_ids.size() - 1; i >= 0; i--)
		{
			if (_my_type_ids[i] == type_id)
			{
				return true;
			}
		}

		return false;
	}

	template<typename TType>
	bool is_of_type() const
	{
		TypeId type_id = extract_type<TType>();

		return is_of_type(type_id);
	}

private:
	std::vector<TypeId> _my_type_ids;
};