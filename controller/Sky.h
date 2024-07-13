#pragma once

#include "Shapes.h"


class Sky
{
public:
	Sky(fisk::tools::V3f aSunDirection, float aSunAngleRadius, fisk::tools::V3f aSunColor, fisk::tools::V3f aSkyColor);


	void BlendWith(fisk::tools::V3f& aInOutColor, fisk::tools::Ray<float, 3> aFrom);

private:

	fisk::tools::V3f mySunDirection;
	float mySunEdge;

	fisk::tools::V3f mySunColor;
	fisk::tools::V3f mySkyColor;
};

