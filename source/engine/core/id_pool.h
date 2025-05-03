#pragma once

#include <vector>

class IdPool
{
public:
	using Id = uint32_t;

	static const Id Invalid = 0;

	IdPool(uint32_t batch_size = 32) : m_batch_size(batch_size)
	{
		assert(batch_size > 0);

		expand_by_one_batch();
	}

	Id acquire_id()
	{
		if (m_ids.size() == 0)
		{
			expand_by_one_batch();
		}

		const Id id = m_ids.back();
		m_ids.pop_back();

		return id;
	}

	void release_id(Id id)
	{
		m_ids.push_back(id);
	}

private:
	void expand_by_one_batch()
	{
		m_ids.reserve(m_ids.size() + m_batch_size);

		const uint32_t first = m_ids.size() + 1;

		for (int32_t i = m_batch_size - 1; i >= 0; --i)
		{
			m_ids.push_back(static_cast<uint32_t>(first + i));
		}		
	}

	const uint32_t m_batch_size;

	std::vector<uint32_t> m_ids;
};