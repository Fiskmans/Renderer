#include "Camera.h"

#include <random>

Camera::Camera(fisk::tools::V2ui aScreenSize, fisk::tools::Ray<float, 3> aAim, float aXFov, Lens aLens)
{
	myScreenSize = aScreenSize;
	myPosition = aAim.myOrigin;

	myLensPlane = fisk::tools::Plane<float>::FromPointandNormal(aAim.PointAt(aLens.myDistance), -aAim.myDirection);

	myFocalpoint = aAim.PointAt(aLens.myFocalLength);

	float xscale = std::tan(aXFov / 2.f) * aLens.myFocalLength;
	float yscale = xscale * static_cast<float>(aScreenSize[1]) / static_cast<float>(aScreenSize[0]);

	fisk::tools::V3f referenceAxis{ 0, 1, 0 };

	if (aAim.myDirection.DistanceSqr(referenceAxis) < 0.01f) // math breaks down
		referenceAxis = { 1, 0, 0 };

	myCameraRight = aAim.myDirection.Cross(referenceAxis).GetNormalized() * xscale / static_cast<float>(aScreenSize[0]);
	myCameraUp = aAim.myDirection.Cross(myCameraRight).GetNormalized() * yscale / static_cast<float>(aScreenSize[1]);

	myLensRight = myCameraRight.GetNormalized() * aLens.myRadius / aLens.myFStop;
	myLensUp = myCameraUp.GetNormalized() * aLens.myRadius / aLens.myFStop;
}

/*

fisk::tools::V2f Raytracer::NormalizedUV(fisk::tools::V2ui aUV)
{
	int scaledX = ;
	int scaledY = static_cast<int>(aUV[1] * 2) - mySize[1];

	float x = static_cast<float>(scaledX) / static_cast<float>(mySize[0]);
	float y = static_cast<float>(scaledY) / static_cast<float>(mySize[1]);

	return { x, y };
}
*/

Camera::Result Camera::Render(fisk::tools::V2ui aUV) const
{
	thread_local std::random_device seed;
	thread_local std::mt19937 rng(seed());
	thread_local std::uniform_real_distribution<float> uniformDist(0, 1);
	thread_local std::normal_distribution<float> normalDist(0, 1);



	fisk::tools::V3f target = myFocalpoint
		+ myCameraRight * (static_cast<float>(aUV[0] * 2) - myScreenSize[0] + uniformDist(rng))
		+ myCameraUp * (static_cast<float>(aUV[1] * 2) - myScreenSize[1] + uniformDist(rng));

	fisk::tools::Ray<float, 3> perfectRay = fisk::tools::Ray<float, 3>::FromPointandTarget(myPosition, target);

	fisk::tools::V3f lensSource = perfectRay.PointAt(*Intersect(perfectRay, myLensPlane));

	fisk::tools::V3f focallyDistortedSource = 
		  lensSource 
		+ myLensRight * normalDist(rng)
		+ myLensUp * normalDist(rng);

	fisk::tools::Ray<float, 3> out = fisk::tools::Ray<float, 3>::FromPointandTarget(focallyDistortedSource, target);

	return out;
}

std::optional<fisk::tools::V2f> Camera::GetScreenPos(fisk::tools::V3f aPoint)
{
	fisk::tools::Ray<float, 3> ray = fisk::tools::Ray<float, 3>::FromPointandTarget(myPosition, aPoint);

	fisk::tools::Plane<float> focalPlane = fisk::tools::Plane<float>::FromPointandNormal(myFocalpoint, myLensPlane.myNormal);

	std::optional<float> dist = fisk::tools::Intersect(ray, focalPlane);

	if (!dist)
		return {};

	fisk::tools::V3f pointInFocalPlane = ray.PointAt(*dist);
	fisk::tools::V3f deltaInPlane = pointInFocalPlane - myFocalpoint;

	float xScale = myCameraRight.Length();
	float yScale = myCameraUp.Length();

	float x = deltaInPlane.Dot(myCameraRight) / xScale / xScale;
	float y = deltaInPlane.Dot(myCameraUp) / yScale / yScale;

	return { { (x + myScreenSize[0]) / 2, (y + myScreenSize[1])/2}};
}

fisk::tools::V3f Camera::GetPosition()
{
	return myPosition;
}

fisk::tools::V3f Camera::GetFocalpoint()
{
	return myFocalpoint;
}

fisk::tools::V3f Camera::GetCameraRight()
{
	return myCameraRight;
}

fisk::tools::V3f Camera::GetCameraUp()
{
	return myCameraUp;
}

bool Camera::Process(fisk::tools::DataProcessor& aProcessor)
{
	return aProcessor.Process(myScreenSize)
		&& aProcessor.Process(myLensPlane)
		&& aProcessor.Process(myPosition)
		&& aProcessor.Process(myFocalpoint)
		&& aProcessor.Process(myCameraRight)
		&& aProcessor.Process(myCameraUp)
		&& aProcessor.Process(myLensRight)
		&& aProcessor.Process(myLensUp);
}
