#pragma once

#include "tools/MathVector.h"

class Material;

struct Hit
{
	fisk::tools::V3f myPosition;
	fisk::tools::V3f myNormal;
	Material* myMaterial;

	unsigned int myObjectId;
};