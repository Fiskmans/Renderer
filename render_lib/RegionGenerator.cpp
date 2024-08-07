#include "RegionGenerator.h"
#include "imgui/imgui.h"

fisk::tools::V2ui RegionGenerator::Get()
{
	return myAtPostion;
}

RegionGenerator::RegionGenerator(fisk::tools::V2ui aSize)
	: mySize(aSize)
	, myAtPostion{ 0, 0 }
{
}

bool RegionGenerator::Next()
{
	if (Done())
		return false;

	myAtPostion[0]++;

	if (myAtPostion[0] == mySize[0])
	{
		myAtPostion[0] = 0;
		myAtPostion[1]++;

		if (myAtPostion[1] == mySize[1])
			return false;

		return true;
	}


	return true;
}

bool RegionGenerator::Done()
{
	if (myAtPostion[1] == mySize[1])
		return true;

	return false;
}