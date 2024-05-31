#pragma once

#include <chrono>
#include "core/reflectable.h"

class TimeManager : public Reflectable
{
public:
	using ParentType = Reflectable;

	std::vector<TypeId> get_types() const override
	{
		static std::vector<TypeId> type_ids = register_type_and_get_types<TimeManager, ParentType>();
		return type_ids;
	}

	void init()
	{
		_start_time_point = std::chrono::system_clock::now();
	}

	float get_elapsed_time() const
	{
		auto now_time_point = std::chrono::system_clock::now();

		std::chrono::duration<float, std::milli> elapsed = now_time_point - _start_time_point;

		float res = elapsed.count() / 1'000;

		return res;
	}

private:
	std::chrono::system_clock::time_point _start_time_point;
};