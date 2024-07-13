#pragma once

#include "tools/Shapes.h"
#include "tools/MathVector.h"
#include "Material.h"

#include <optional>

class IIntersector
{
public:
	virtual ~IIntersector() = default;

	virtual std::optional<Hit> Intersect(fisk::tools::Ray<float, 3> aRay) = 0;
};

