#pragma once

#include "Scene.h"
#include "IIntersector.h"

class DumbIntersector : public IIntersector
{
public:
	DumbIntersector(const Scene& aScene);

	std::optional<Hit> Intersect(fisk::tools::Ray<float, 3> aRay) override;

private:
	const Scene& myScene;
};

