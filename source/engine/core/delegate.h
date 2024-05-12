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

	DelegateFunctionNoParams(FunctionType func) : func_(func) {}

	TReturn operator()() override
	{
		return func_();
	}

private:
	FunctionType func_;
};

template<typename TReturn>
class DelegateFunctionNoParams<TReturn, typename std::enable_if<std::is_same<TReturn, void>::value>::type> : public Delegate<TReturn>
{
public:
	using FunctionType = void (*)();

	DelegateFunctionNoParams(FunctionType func) : func_(func) {}

	void operator()() override
	{
		func_();
	}

private:
	FunctionType func_;
};
