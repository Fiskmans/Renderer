#include "Scene.h"

#include <cassert>

#include "tools/Logger.h"
#include "tools/Iterators.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <iostream>

float operator ""_m(unsigned long long aValue)
{
	return static_cast<float>(aValue);
}

float operator ""_cm(unsigned long long aValue)
{
	return aValue / 100.f;
}

float operator ""_deg(unsigned long long aValue)
{
	return aValue * 0.0174532f;
}

Scene::Scene()
	: myIdCounter(0)
{
}

bool Scene::Process(fisk::tools::DataProcessor& aProcessor)
{
	return aProcessor.Process(myMaterials)
		&& aProcessor.Process(myPolyObjects)
		&& aProcessor.Process(myCamera)
		&& aProcessor.Process(mySky);
}

void Scene::Add(const PolyObject& aPolyObject, size_t aMaterialIndex)
{
	SceneObject<PolyObject> scenePoly;

	scenePoly.myShape = aPolyObject;
	scenePoly.myId = ++myIdCounter;
	scenePoly.myMaterialIndex = static_cast<unsigned int>(aMaterialIndex);

	myPolyObjects.push_back(scenePoly);
}

std::unique_ptr<Scene> Scene::FromFile(std::string aFilePath, fisk::tools::V2ui aResolution)
{
	std::unique_ptr<Scene> out = std::make_unique<Scene>();

	Assimp::Importer importer;

	const aiScene* scene = importer.ReadFile(aFilePath,
								aiProcess_Triangulate |
								aiProcess_JoinIdenticalVertices | 
								aiProcess_DropNormals | 
								aiProcess_GenBoundingBoxes | 
								aiProcess_ValidateDataStructure);

	if (!scene)
	{
		LOG_ERROR("Failed to load scene", importer.GetErrorString());
		return {};
	}

	out->ImportMaterials(scene);
	out->ImportNode(scene, scene->mRootNode);
	out->ImportCamera(scene, aResolution);
	out->ImportSky();


	return out;
}

const std::vector<SceneObject<PolyObject>>& Scene::GetObjects() const
{
	return myPolyObjects;
}

const Camera& Scene::GetCamera() const
{
	assert(myCamera);
	return *myCamera;
}

const Sky& Scene::GetSky() const
{
	assert(mySky);
	return *mySky;
}

const Material* Scene::GetMaterial(size_t aMaterialIndex) const
{
	return myMaterials[aMaterialIndex].get();
}

void Scene::ImportMaterials(const aiScene* aScene)
{
	if (!aScene->HasMaterials())
		return;

	for (size_t i = 0; i < aScene->mNumMaterials; i++)
	{
		const aiMaterial* aiMat = aScene->mMaterials[i];
		std::unique_ptr<Material> mat = std::make_unique<Material>();

		LOG_INFO(aiMat->GetName().C_Str());

		mat->myColor = { 0.8f, 0.8f, 0.8f };
		mat->mySpecular = 0.5f;

		aiColor3D diffuse;
		if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS)
			mat->myColor = { diffuse.r, diffuse.g, diffuse.b };
		/*

		float metalness;
		if (aiMat->Get(AI_MATKEY_METALLIC_FACTOR, metalness) == AI_SUCCESS)
			mat->mySpecular = metalness;*/

		float roughness;
		if (aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
			mat->mySpecular = 1.f - roughness;

		myMaterials.push_back(std::move(mat));
	}
}

void Scene::ImportNode(const aiScene* aScene, const aiNode* aNode)
{
	ImportMeshes(aScene->mMeshes, aNode);

	if (!aNode->mChildren)
		return;

	for (const aiNode* child : fisk::tools::RangeFromStartEnd(aNode->mChildren, aNode->mChildren + aNode->mNumChildren))
	{
		ImportNode(aScene, child);
	}
}

void Scene::ImportMeshes(aiMesh*const* aMeshes, const aiNode* aNode)
{
	if (!aNode->mMeshes)
		return;

	aiMatrix4x4 transform = FullTransform(aNode);

	for (unsigned int meshIndex : fisk::tools::RangeFromStartEnd(aNode->mMeshes, aNode->mMeshes + aNode->mNumMeshes))
	{
		const aiMesh* mesh = aMeshes[meshIndex];

		if (!mesh->HasFaces())
			return;
		
		PolyObject obj = PolyObject::FromTri(TriFromFace(mesh->mVertices, mesh->mFaces[0], transform));

		for (const aiFace& face : fisk::tools::RangeFromStartEnd(mesh->mFaces + 1, mesh->mFaces + mesh->mNumFaces))
			obj.AddTri(TriFromFace(mesh->mVertices, face, transform));

		Add(obj, mesh->mMaterialIndex);
	}
}

void Scene::ImportSky()
{
	mySky.emplace(
		fisk::tools::V3f{ 1, 1, 1 }, 
		23_deg, 
		fisk::tools::V3f{ 1462.436f, 1049.146f, 303.80522f }, 
		fisk::tools::V3f{ 178.5f, 229.5f, 255.f });

}

void Scene::ImportCamera(const aiScene* aScene, fisk::tools::V2ui aResolution)
{
	if (!aScene->HasCameras())
	{
		Camera::Lens lens;

		lens.myFocalLength = 20_m;
		lens.myDistance = 5_cm;
		lens.myRadius = 5_cm;
		lens.myFStop = 1 / 1.f;

		myCamera.emplace(aResolution, fisk::tools::Ray<float, 3>::FromPointandTarget({ 2_m, 6_m, 8_m }, { 0_m, 0_m, 0_m }), 100_deg, lens);
		return;
	}

	for (const aiCamera* camera : fisk::tools::RangeFromStartEnd(aScene->mCameras, aScene->mCameras + aScene->mNumCameras))
	{
		const aiNode* node = FindNodeByName(aScene->mRootNode, camera->mName);

		aiMatrix4x4 transform = FullTransform(node);

		for (unsigned int i = 0; i < node->mMetaData->mNumProperties; i++)
		{
			aiString name = node->mMetaData->mKeys[i];
			const aiMetadataEntry data = node->mMetaData->mValues[i];

			std::cout << "Key: " << name.C_Str() << std::endl;

			switch (data.mType)
			{
			case AI_AISTRING:
				std::cout << "Data: " << reinterpret_cast<const aiString*>(data.mData)->C_Str() << std::endl;
				break;
			}

		}

		Camera::Lens lens;

		lens.myFocalLength = 20_m;
		lens.myDistance = 5_cm;
		lens.myRadius = 5_cm;
		lens.myFStop = 100 / 1.f;

		fisk::tools::Ray<float, 3> direction;
		
		direction.myOrigin = TranslateVectorType(transform * camera->mPosition);
		direction.myDirection = -TranslateVectorType(transform * camera->mLookAt).GetNormalized();

		myCamera.emplace(aResolution, 
			direction,
			camera->mHorizontalFOV * 2, 
			lens);
	}
}

fisk::tools::Tri<float> Scene::TriFromFace(const aiVector3D* aVerticies, const aiFace& aFace, const aiMatrix4x4& aTransform)
{
	assert(aFace.mNumIndices == 3 && "Mesh was not triangularized properly");

	return fisk::tools::Tri<float>::FromCorners(
		TranslateVectorType(aTransform * aVerticies[aFace.mIndices[0]]),
		TranslateVectorType(aTransform * aVerticies[aFace.mIndices[1]]),
		TranslateVectorType(aTransform * aVerticies[aFace.mIndices[2]]));
}

fisk::tools::V3f Scene::TranslateVectorType(const aiVector3D& aVector)
{
	return { aVector.x, aVector.y, aVector.z };
}

const aiNode* Scene::FindNodeByName(const aiNode* aNode, const aiString& aName)
{
	if (aNode->mName == aName)
		return aNode;

	for (const aiNode* node : fisk::tools::RangeFromStartEnd(aNode->mChildren, aNode->mChildren + aNode->mNumChildren))
	{
		const aiNode* subNode = FindNodeByName(node, aName);
		if (subNode)
			return subNode;
	}

	return nullptr;
}

aiMatrix4x4 Scene::FullTransform(const aiNode* aNode)
{
	aiMatrix4x4 transform = aNode->mTransformation;

	const aiNode* at = aNode->mParent;
	while (at)
	{
		transform *= at->mTransformation;
		at = at->mParent;
	}

	return transform;
}
