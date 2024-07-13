#pragma once

#include "tools/MathVector.h"
#include "Shapes.h"
#include "Hit.h"

class Material
{
public:
	fisk::tools::V3f myColor;
	float specular = 0.1f;

	void InteractWith(fisk::tools::Ray<float, 3>& aInOutRay, Hit& aHit, fisk::tools::V3f& aColor);

	void ReflectSpecular(fisk::tools::Ray<float, 3>& aInOutRay, Hit& aHit);
	void ReflectDiffuse(fisk::tools::Ray<float, 3>& aInOutRay, Hit& aHit);
};



