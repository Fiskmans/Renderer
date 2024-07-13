#include "Sky.h"


Sky::Sky(fisk::tools::V3f aSunDirection, float aSunAngleRadius, fisk::tools::V3f aSunColor, fisk::tools::V3f aSkyColor)
{
	mySunDirection = aSunDirection;
	mySunDirection.Normalize();
	
	mySunEdge = std::cos(aSunAngleRadius);

	mySunColor = aSunColor;
	mySkyColor = aSkyColor;
}

void Sky::BlendWith(fisk::tools::V3f& aInOutColor, fisk::tools::Ray<float, 3> aFrom)
{
	float alignment = aFrom.myDirection.Dot(mySunDirection);

	if (alignment > mySunEdge)
		aInOutColor *= mySunColor;
	else
		aInOutColor *= mySkyColor;
}
