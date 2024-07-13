#pragma once


#include "tools/MathVector.h"

#include "Shapes.h"

class Camera
{
public:
	struct Lens
	{
		float myRadius;
		float myDistance;

		float myFocalLength;
		float myFStop;
	};

	Camera(fisk::tools::V2ui aScreenSize, fisk::tools::Ray<float, 3> aAim, float aXFov, Lens aLens);

	fisk::tools::Ray<float, 3> RayAt(fisk::tools::V2ui aUV);

	fisk::tools::V3f GetPosition();
	fisk::tools::V3f GetFocalpoint();
	fisk::tools::V3f GetCameraRight();
	fisk::tools::V3f GetCameraUp();

private:
	fisk::tools::V2ui myScreenSize;

	fisk::tools::Plane<float> myLensPlane;

	fisk::tools::V3f myPosition;
	fisk::tools::V3f myFocalpoint;

	fisk::tools::V3f myCameraRight;
	fisk::tools::V3f myCameraUp;

	fisk::tools::V3f myLensRight;
	fisk::tools::V3f myLensUp;
};