#pragma once

#include <functional>

template<typename TReturn, typename... TParams>
class Delegate
{
public:

	template<typename TFunction>
	Delegate(TFunction function)
	{
		_function = function;
	}

	template<typename TObject, typename TMemberFunction>
	Delegate(TObject* object, TMemberFunction member_function)
	{
		_function = [object, member_function](TParams&&... params)
			{
				return (object->*member_function)(std::forward<TParams>(params)...);
			};
	}

	TReturn operator()(TParams&&... params) const
	{
		return _function(std::forward<TParams>(params)...);
	}

private:
	std::function<TReturn(TParams...)> _function;
};