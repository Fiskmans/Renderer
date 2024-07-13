#include "Material.h"

#include <random>

namespace material_randoms
{
	thread_local std::random_device seed;
	thread_local std::mt19937 rng(seed());
	thread_local std::normal_distribution<float> normalDist(0, 1);
	thread_local std::uniform_real_distribution<float> uniform(0, 1);
}

void Material::InteractWith(fisk::tools::Ray<float, 3>& aInOutRay, Hit& aHit, fisk::tools::V3f& aColor)
{

	if (material_randoms::uniform(material_randoms::rng) < specular)
	{
		ReflectSpecular(aInOutRay, aHit);
		return;
	}

	aColor *= myColor;
	aInOutRay.myOrigin = aHit.myPosition;

	ReflectDiffuse(aInOutRay, aHit);
}

void Material::ReflectSpecular(fisk::tools::Ray<float, 3>& aInOutRay, Hit& aHit)
{
	aInOutRay.myDirection.Reverse();
	aInOutRay.myDirection.ReflectOn(aHit.myNormal);
}

void Material::ReflectDiffuse(fisk::tools::Ray<float, 3>& aInOutRay, Hit& aHit)
{
	fisk::tools::V3f dir
	{
		material_randoms::normalDist(material_randoms::rng),
		material_randoms::normalDist(material_randoms::rng),
		material_randoms::normalDist(material_randoms::rng)
	};

	dir.Normalize();

	if (dir.Dot(aHit.myNormal) < 0.f)
		dir.Reverse();

	aInOutRay.myDirection = dir;
}
