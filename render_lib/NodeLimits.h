#pragma once

#include "tools/DataProcessor.h"

#include <cstdint>

struct NodeLimits
{
	uint32_t myMaxPending;
	uint32_t myThreads;

	inline bool Process(fisk::tools::DataProcessor& aProcessor)
	{
		return aProcessor.Process(myMaxPending)
			&& aProcessor.Process(myThreads);
	}
};