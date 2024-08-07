#pragma once

#include "tools/MathVector.h"

#include <string>

class RegionGenerator
{
public:
	RegionGenerator(fisk::tools::V2ui aSize);

	RegionGenerator(const RegionGenerator&) = default;
	RegionGenerator& operator=(const RegionGenerator&) = default;
	RegionGenerator(RegionGenerator&&) = default;
	RegionGenerator& operator=(RegionGenerator&&) = default;
	
	fisk::tools::V2ui Get();
	bool Done();
	bool Next();

private:
	fisk::tools::V2ui mySize;

	fisk::tools::V2ui myAtPostion;
};

