#pragma once

#include "tools/Shapes.h"

#include "IRenderer.h"
#include "RendererTypes.h"
#include "IIntersector.h"
#include "Sky.h"

class RayRenderer : public IRenderer<TextureType::PackedValues>
{
public:
	static constexpr size_t MaxBounces = 16;
	using RayCaster = IRenderer<fisk::tools::Ray<float, 3>>;

	RayRenderer(RayCaster& aRayCaster, IIntersector& aIntersector, Sky& aSky, size_t aSamplesPerTexel);

	Result Render(fisk::tools::V2ui aUV) override;

private:

	struct Sample
	{
		fisk::tools::V3f myColor{ 1, 1, 1 };
		unsigned int myObjectId = 0;
	};

	Sample SampleTexel(fisk::tools::V2ui aUV);

	RayCaster& myRayCaster;
	IIntersector& myIntersector;
	Sky& mySky;
	size_t mySamplesPerTexel;
};