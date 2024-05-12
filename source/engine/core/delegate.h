#pragma once

template<typename TReturn = void>
class Delegate
{
public:
	virtual ~Delegate() = default;
	virtual TReturn operator()() = 0;
};

template<typename TReturn, typename = void>
class DelegateFunctionNoParams : public Delegate<TReturn>
{
public:
	using FunctionType = TReturn(*)();

	DelegateFunctionNoParams(FunctionType function) : _function(function) {}

	TReturn operator()() override
	{
		return _function();
	}

private:
	FunctionType _function;
};

template<typename TReturn>
class DelegateFunctionNoParams<TReturn, typename std::enable_if<std::is_same<TReturn, void>::value>::type> : public Delegate<TReturn>
{
public:
	using FunctionType = void (*)();

	DelegateFunctionNoParams(FunctionType function) : _function(function) {}

	void operator()() override
	{
		_function();
	}

private:
	FunctionType _function;
};

template<typename TReturn = void, typename TParam1 = int>
class DelegateOneParam
{
public:
	virtual ~DelegateOneParam() = default;
	virtual TReturn operator()(TParam1) = 0;
};

template<typename TReturn, typename TParam1, typename = void>
class DelegateFunctionOneParam : public DelegateOneParam<TReturn, TParam1>
{
public:
	using FunctionType = TReturn(*)(TParam1);

	DelegateFunctionOneParam(FunctionType function) : _function(function) {}

	TReturn operator()(TParam1 param1) override
	{
		return _function(param1);
	}

private:
	FunctionType _function;
};

template<typename TReturn, typename TParam1>
class DelegateFunctionOneParam<TReturn, TParam1, typename std::enable_if<std::is_same<TReturn, void>::value>::type> : public DelegateOneParam<TReturn, TParam1>
{
public:
	using FunctionType = void (*)(TParam1);

	DelegateFunctionOneParam(FunctionType function) : _function(function) {}

	void operator()(TParam1 param1) override
	{
		_function(param1);
	}

private:
	FunctionType _function;
};