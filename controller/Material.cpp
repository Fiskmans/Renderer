#include "Material.h"

#include <random>

namespace material_randoms
{
	thread_local std::random_device seed;
	thread_local std::mt19937 rng(seed());
	thread_local std::normal_distribution<float> normalDist(0, 1);
	thread_local std::uniform_real_distribution<float> uniform(0, 1);
}

void Material::InteractWith(fisk::tools::Ray<float, 3>& aInOutRay, Hit& aHit, fisk::tools::V3f& aColor) const
{

	if (material_randoms::uniform(material_randoms::rng) < mySpecular)
	{
		ReflectSpecular(aInOutRay, aHit);
		return;
	}

	aColor *= myColor;
	aInOutRay.myOrigin = aHit.myPosition;

	ReflectDiffuse(aInOutRay, aHit, aColor);
}

void Material::ReflectSpecular(fisk::tools::Ray<float, 3>& aInOutRay, Hit& aHit) const
{
	aInOutRay.myDirection.Reverse();
	aInOutRay.myDirection.ReflectOn(aHit.myNormal);
}

void Material::ReflectDiffuse(fisk::tools::Ray<float, 3>& aInOutRay, Hit& aHit, fisk::tools::V3f& aColor) const
{
	aInOutRay.myOrigin = aHit.myPosition;
	aInOutRay.myDirection = (aHit.myNormal + fisk::tools::V3f(
												material_randoms::normalDist(material_randoms::rng),
												material_randoms::normalDist(material_randoms::rng),
												material_randoms::normalDist(material_randoms::rng))).GetNormalized();
}

bool Material::Process(fisk::tools::DataProcessor& aProcessor)
{
	bool success = true;

	success &= aProcessor.Process(myColor);
	success &= aProcessor.Process(mySpecular);
	
	return success;
}
