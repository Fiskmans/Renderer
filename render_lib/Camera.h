#pragma once


#include "tools/DataProcessor.h"
#include "tools/MathVector.h"
#include "tools/Shapes.h"

#include "IRenderer.h"

#include <optional>

class Camera : public IRenderer<fisk::tools::Ray<float, 3>>
{
public:
	struct Lens
	{
		float myRadius;
		float myDistance;

		float myFocalLength;
		float myFStop;
	};

	Camera() = default;

	Camera(fisk::tools::V2ui aScreenSize, fisk::tools::Ray<float, 3> aAim, float aXFov, Lens aLens);

	Result Render(fisk::tools::V2ui aUV) const;

	std::optional<fisk::tools::V2f> GetScreenPos(fisk::tools::V3f aPoint);

	fisk::tools::V3f GetPosition();
	fisk::tools::V3f GetFocalpoint();
	fisk::tools::V3f GetCameraRight();
	fisk::tools::V3f GetCameraUp();

	bool Process(fisk::tools::DataProcessor& aProcessor);

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