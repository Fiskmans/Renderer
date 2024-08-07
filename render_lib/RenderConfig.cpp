#pragma once

#include "RenderConfig.h"

bool RenderConfig::Process(fisk::tools::DataProcessor& aProcessor)
{
	return aProcessor.Process(myMode)
		&& aProcessor.Process(mySamplesPerTexel)
		&& aProcessor.Process(myRenderId);
}