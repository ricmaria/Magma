#pragma once

template<typename TReturn, typename... TParams>
class Delegate
{
public:
	virtual ~Delegate() = default;
	virtual TReturn operator()(TParams...) = 0;
};

template<typename TReturn, typename... TParams>
class DelegateFunction : public Delegate<TReturn, TParams...>
{
public:
	using FunctionType = TReturn(*)(TParams...);

	DelegateFunction(FunctionType function) : _function(function) {}

	TReturn operator()(TParams... params) override
	{
		return _function(params...);
	}

private:
	FunctionType _function;
};

template<typename TReturn, typename TObject, typename... TParams>
class DelegateMethod : public Delegate<TReturn, TParams...>
{
public:
	using MethodType = TReturn(TObject::*)(TParams...);

	DelegateMethod(TObject* object, MethodType method) :
		_object(object), _method(method) {}

	TReturn operator()(TParams... params) override
	{
		return (_object->*_method)(params...);
	}

private:
	TObject* _object;
	MethodType _method;
};