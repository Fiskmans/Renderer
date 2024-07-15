
#pragma once

#include "tools/Shapes.h"
#include <vector>


struct PolyObject
{
	std::vector<fisk::tools::Tri<float>> myTris;
	fisk::tools::AxisAlignedBox<float, 3> myBoundingBox;

	static PolyObject FromTri(fisk::tools::Tri<float> aTri);
	void AddTri(fisk::tools::Tri<float> aTri);
};