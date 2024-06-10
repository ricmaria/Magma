#pragma once

#include <vector>

class IdPool
{
public:
	using Id = uint32_t;

	IdPool(uint32_t batch_size = 32) : _batch_size(batch_size)
	{
		assert(batch_size > 0);

		expand_by_one_batch();
	}

	Id acquire_id()
	{
		if (_ids.size() == 0)
		{
			expand_by_one_batch();
		}

		const Id id = _ids.back();
		_ids.pop_back();

		return id;
	}

	void release_id(Id id)
	{
		_ids.push_back(id);
	}

private:
	void expand_by_one_batch()
	{
		_ids.reserve(_ids.size() + _batch_size);

		const uint32_t first = _ids.size();

		for (int32_t i = _batch_size - 1; i >= 0; --i)
		{
			_ids.push_back(static_cast<uint32_t>(first + i));
		}		
	}

	const uint32_t _batch_size;

	std::vector<uint32_t> _ids;
};