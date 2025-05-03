#pragma once

#include <chrono>
#include "core/reflectable.h"

class TimeManager
{
public:

	void init()
	{
		m_start_time_point = std::chrono::system_clock::now();
	}

	float get_elapsed_time() const
	{
		auto now_time_point = std::chrono::system_clock::now();

		std::chrono::duration<float, std::milli> elapsed = now_time_point - m_start_time_point;

		float res = elapsed.count() / 1'000;

		return res;
	}

private:
	std::chrono::system_clock::time_point m_start_time_point;
};