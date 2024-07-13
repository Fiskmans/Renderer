#pragma once

#include <vector>

#include "Shapes.h"
#include "Material.h"

template<class Shape>
struct SceneObject
{
	Shape myShape;
	unsigned int myId;
	Material* myMaterial;
};


class Scene
{
public:
	Scene();

	void Add(const fisk::tools::Sphere<float>& aSphere, Material* aMaterial);
	void Add(const fisk::tools::Plane<float>& aPlane, Material* aMaterial);

	std::vector<SceneObject<fisk::tools::Sphere<float>>> mySpheres;
	std::vector<SceneObject<fisk::tools::Plane<float>>> myPlanes;

private:
	unsigned int myIdCounter;

};

