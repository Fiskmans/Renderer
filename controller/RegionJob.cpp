#include "RegionJob.h"
#include "imgui/imgui.h"

RegionJob::RegionJob(fisk::tools::V2ui aTopLeft, fisk::tools::V2ui aBottomRight, size_t aSamplesPerTexel, std::string aName)
	: myTopLeft(aTopLeft)
	, myBottomRight(aBottomRight)
	, mySamplesPerTexel(aSamplesPerTexel)
	, myName(aName)
	, myAtPostion(aTopLeft)
	, myAtSample(0)
{
}

fisk::tools::V2ui RegionJob::Get()
{
	return myAtPostion;
}

bool RegionJob::Next()
{
	if (Done())
		return false;

	if (++myAtSample < mySamplesPerTexel)
		return true;

	myAtSample = 0;

	myAtPostion[0]++;

	if (myAtPostion[0] == myBottomRight[0])
	{
		myAtPostion[0] = myTopLeft[0];
		myAtPostion[1]++;

		if (myAtPostion[1] == myBottomRight[1])
			return false;

		return true;
	}


	return true;
}

bool RegionJob::Done()
{
	if (myAtPostion[1] == myBottomRight[1])
		return true;

	return false;
}

void RegionJob::Imgui()
{
	ImGui::Text("%s [%u]", myName.c_str(), mySamplesPerTexel);

	float progress = static_cast<float>(myAtPostion[1] - myTopLeft[1]) / static_cast<float>(myBottomRight[1] - myTopLeft[1]);

	ImGui::ProgressBar(progress);
	ImGui::Text("x1:%u x2:%u ", myTopLeft[0], myBottomRight[0]);
	ImGui::Text("y1:%u y2:%u ", myTopLeft[1], myBottomRight[1]);
}

std::string RegionJob::GetName() const
{
	return myName;
}

fisk::tools::V2ui RegionJob::GetTopLeft() const
{
	return myTopLeft;
}

fisk::tools::V2ui RegionJob::GetBottomRight() const
{
	return myBottomRight;
}
