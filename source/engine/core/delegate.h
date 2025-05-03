#pragma once

#include <functional>

template<typename TReturn, typename... TParams>
class Delegate
{
public:

	template<typename TFunction>
	Delegate(TFunction function)
	{
		m_function = function;
	}

	template<typename TObject, typename TMemberFunction>
	Delegate(TObject* object, TMemberFunction member_function)
	{
		m_function = [object, member_function](TParams&&... params)
			{
				return (object->*member_function)(std::forward<TParams>(params)...);
			};
	}

	TReturn operator()(TParams&&... params) const
	{
		return m_function(std::forward<TParams>(params)...);
	}

private:
	std::function<TReturn(TParams...)> m_function;
};