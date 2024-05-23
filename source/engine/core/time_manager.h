#pragma once

#include <chrono>

class TimeManager
{
public:
	void init()
	{
		_start_time_point = std::chrono::steady_clock::now();
	}

	float get_elapsed_time() const
	{
		auto now_time_point = std::chrono::steady_clock::now();

		float res = std::chrono::duration_cast<std::chrono::seconds>(now_time_point - _start_time_point).count();

		return res;
	}

private:
	std::chrono::steady_clock::time_point _start_time_point;
};