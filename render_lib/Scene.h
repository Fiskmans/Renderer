#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

#include "tools/Concepts.h"
#include "tools/DataProcessor.h"
#include "tools/Shapes.h"
#include "Material.h"
#include "PolyObject.h"
#include "IRenderer.h"
#include "Sky.h"
#include "Camera.h"

#include "assimp/material.h"
#include "assimp/scene.h"

template<class Shape>
struct SceneObject
{
	unsigned int myId;
	unsigned int myMaterialIndex;
	Shape myShape;

	bool Process(fisk::tools::DataProcessor& aProcessor);
};

class Scene
{
public:
	Scene();

	bool Process(fisk::tools::DataProcessor& aProcessor);

	const std::vector<SceneObject<PolyObject>>& GetObjects() const;
	const Camera& GetCamera() const;
	const Sky& GetSky() const;
	const Material* GetMaterial(size_t aMaterialIndex) const;

	static std::unique_ptr<Scene> FromFile(std::string aFilePath, fisk::tools::V2ui aResolution);

private:

	void ImportMaterials(const aiScene* aScene);
	void ImportNode(const aiScene* aScene, const aiNode* aNode);
	void ImportMeshes(aiMesh* const * aMeshes, const aiNode* aNode);
	void ImportCamera(const aiScene* aScene, fisk::tools::V2ui aResolution);
	void ImportSky();


	void Add(const PolyObject& aPolyObject, size_t aMaterialIndex);

	static fisk::tools::Tri<float> TriFromFace(const aiVector3D* aVerticies, const aiFace& aFace, const aiMatrix4x4& aTransform);
	static fisk::tools::V3f TranslateVectorType(const aiVector3D& aVector);
	static const aiNode* FindNodeByName(const aiNode* aNode, const aiString& aName);
	static aiMatrix4x4 FullTransform(const aiNode* aNode);

	std::vector<std::unique_ptr<Material>> myMaterials;
	std::vector<SceneObject<PolyObject>> myPolyObjects;
	std::optional<Camera> myCamera;
	std::optional<Sky> mySky;

	unsigned int myIdCounter;
};

template<class Shape>
inline bool SceneObject<Shape>::Process(fisk::tools::DataProcessor& aProcessor)
{
	return aProcessor.Process(myId)
		&& aProcessor.Process(myMaterialIndex)
		&& aProcessor.Process(myShape);
}