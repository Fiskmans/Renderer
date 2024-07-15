#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

#include "tools/Shapes.h"
#include "Material.h"
#include "PolyObject.h"

#include "assimp/material.h"
#include "assimp/scene.h"

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
	~Scene();

	Scene(const Scene&) = delete;
	Scene(Scene&&) = delete;
	Scene& operator=(const Scene&) = delete;
	Scene& operator=(Scene&&) = delete;

	void Add(const fisk::tools::Sphere<float>& aSphere, Material* aMaterial);
	void Add(const fisk::tools::Plane<float>& aPlane, Material* aMaterial);
	void Add(const fisk::tools::Tri<float>& aTri, Material* aMaterial);
	void Add(const PolyObject& aPolyObject, Material* aMaterial);

	std::vector<SceneObject<fisk::tools::Sphere<float>>> mySpheres;
	std::vector<SceneObject<fisk::tools::Plane<float>>> myPlanes;
	std::vector<SceneObject<fisk::tools::Tri<float>>> myTris;
	std::vector<SceneObject<PolyObject>> myPolyObjects;

	static std::unique_ptr<Scene> FromFile(std::string aFilePath);

private:

	void ImportMaterials(const aiScene* aScene);

	void ImportNode(const aiScene* aScene, const aiNode* aNode);

	void ImportMeshes(aiMesh* const * aMeshes, const aiNode* aNode, const aiMatrix4x4& aTransform);

	static fisk::tools::Tri<float> TriFromFace(const aiVector3D* aVerticies, const aiFace& aFace, const aiMatrix4x4& aTransform);

	static fisk::tools::V3f TranslateVectorType(const aiVector3D& aVector);

	std::vector<Material*> myMaterials;

	unsigned int myIdCounter;
};

