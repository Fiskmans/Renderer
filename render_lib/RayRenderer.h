#pragma once

#include "tools/Shapes.h"

#include "Scene.h"
#include "IRenderer.h"
#include "RendererTypes.h"
#include "IIntersector.h"
#include "Sky.h"

class RayRenderer : public IRenderer<TextureType::PackedValues>
{
public:
	static constexpr size_t MaxBounces = 16;
	using RayCaster = IRenderer<fisk::tools::Ray<float, 3>>;

	RayRenderer(const Scene& aScene, IIntersector& aIntersector, size_t aSamplesPerTexel, unsigned int aRendererId);

	Result Render(fisk::tools::V2ui aUV) const override;

private:

	struct Sample
	{
		fisk::tools::V3f myColor{ 1, 1, 1 };
		unsigned int myObjectId = 0;
		unsigned int mySubObjectId = 0;
	};

	Sample SampleTexel(fisk::tools::V2ui aUV) const;

	const RayCaster& myRayCaster;
	IIntersector& myIntersector;
	const Sky& mySky;
	size_t mySamplesPerTexel;
	unsigned int myRendererId;
};