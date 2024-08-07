#pragma once

#include "tools/DataProcessor.h"
#include "tools/Shapes.h"


class Sky
{
public:
	Sky() = default;
	Sky(fisk::tools::V3f aSunDirection, float aSunAngleRadius, fisk::tools::V3f aSunColor, fisk::tools::V3f aSkyColor);

	bool Process(fisk::tools::DataProcessor& aProcessor);

	void BlendWith(fisk::tools::V3f& aInOutColor, fisk::tools::Ray<float, 3> aFrom) const;

private:

	fisk::tools::V3f mySunDirection;
	float mySunEdge;

	fisk::tools::V3f mySunColor;
	fisk::tools::V3f mySkyColor;
};

