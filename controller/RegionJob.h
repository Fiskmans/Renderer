#pragma once

#include "tools/MathVector.h"

#include <string>

class RegionJob
{
public:
	RegionJob(fisk::tools::V2ui aTopLeft, fisk::tools::V2ui aBottomRight, size_t aSamplesPerTexel, std::string aName = "unnamed");
	RegionJob(const RegionJob&) = default;
	RegionJob& operator=(const RegionJob&) = default;
	RegionJob(RegionJob&&) = default;
	RegionJob& operator=(RegionJob&&) = default;
	
	fisk::tools::V2ui Get();
	bool Next();
	bool Done();
	void Imgui();


	std::string GetName() const;
	fisk::tools::V2ui GetTopLeft() const;
	fisk::tools::V2ui GetBottomRight() const;

private:

	fisk::tools::V2ui myTopLeft;
	fisk::tools::V2ui myBottomRight;

	size_t mySamplesPerTexel;

	std::string myName;

	fisk::tools::V2ui myAtPostion;
	size_t myAtSample;
};

