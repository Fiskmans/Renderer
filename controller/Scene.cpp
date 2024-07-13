#include "Scene.h"

Scene::Scene()
	: myIdCounter(0)
{
}

void Scene::Add(const fisk::tools::Sphere<float>& aSphere, Material* aMaterial)
{
	SceneObject<fisk::tools::Sphere<float>> sceneSphere;

	sceneSphere.myShape = aSphere;
	sceneSphere.myId = ++myIdCounter;
	sceneSphere.myMaterial = aMaterial;

	mySpheres.push_back(sceneSphere);
}

void Scene::Add(const fisk::tools::Plane<float>& aPlane, Material* aMaterial)
{
	SceneObject<fisk::tools::Plane<float>> scenePlane;

	scenePlane.myShape = aPlane;
	scenePlane.myId = ++myIdCounter;
	scenePlane.myMaterial = aMaterial;

	myPlanes.push_back(scenePlane);
}
