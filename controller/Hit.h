#pragma once

#include "tools/MathVector.h"

class Material;

struct Hit
{
	fisk::tools::V3f myPosition{};
	fisk::tools::V3f myNormal{};
	Material* myMaterial = nullptr;

	unsigned int myObjectId = 0;
	unsigned int mySubObjectId = 0;
};