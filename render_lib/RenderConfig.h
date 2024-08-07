#pragma once

#include "tools/DataProcessor.h"

#include <cstdint>

struct RenderConfig
{
	enum RenderMode : uint32_t
	{
		RaytracedClustered
	};

	bool Process(fisk::tools::DataProcessor& aProcessor);

	RenderMode myMode;
	size_t mySamplesPerTexel;
	unsigned int myRenderId;
};