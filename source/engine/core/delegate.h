#pragma once

class Delegate
{
public:
	virtual ~Delegate() = default;
	virtual void operator()() {};
};

//template<typename T>
//class DelegateObjectNoParams()
//{
//
//}
