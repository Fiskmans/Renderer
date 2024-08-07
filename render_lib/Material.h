#pragma once

#include "tools/DataProcessor.h"
#include "tools/MathVector.h"
#include "tools/Shapes.h"
#include "Hit.h"

class Material
{
public:
	fisk::tools::V3f myColor;
	float mySpecular = 0.1f;

	void InteractWith(fisk::tools::Ray<float, 3>& aInOutRay, Hit& aHit, fisk::tools::V3f& aColor) const;

	void ReflectSpecular(fisk::tools::Ray<float, 3>& aInOutRay, Hit& aHit) const;
	void ReflectDiffuse(fisk::tools::Ray<float, 3>& aInOutRay, Hit& aHit, fisk::tools::V3f& aColor) const;

	bool Process(fisk::tools::DataProcessor& aProcessor);
};

