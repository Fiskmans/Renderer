#include "Scene.h"

#include <cassert>

#include "tools/Logger.h"
#include "tools/Iterators.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

Scene::Scene()
	: myIdCounter(0)
{
}

Scene::~Scene()
{
	for (Material* mat : myMaterials)
		delete mat;
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

void Scene::Add(const fisk::tools::Tri<float>& aTri, Material* aMaterial)
{
	SceneObject<fisk::tools::Tri<float>> sceneTri;

	sceneTri.myShape = aTri;
	sceneTri.myId = ++myIdCounter;
	sceneTri.myMaterial = aMaterial;

	myTris.push_back(sceneTri);
}

void Scene::Add(const PolyObject& aPolyObject, Material* aMaterial)
{
	SceneObject<PolyObject> scenePoly;

	scenePoly.myShape = aPolyObject;
	scenePoly.myId = ++myIdCounter;
	scenePoly.myMaterial = aMaterial;

	myPolyObjects.push_back(scenePoly);
}

std::unique_ptr<Scene> Scene::FromFile(std::string aFilePath)
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

	return out;
}

void Scene::ImportMaterials(const aiScene* aScene)
{
	if (!aScene->HasMaterials())
		return;

	for (size_t i = 0; i < aScene->mNumMaterials; i++)
	{
		const aiMaterial* aiMat = aScene->mMaterials[i];
		Material* mat = new Material();

		LOG_INFO(aiMat->GetName().C_Str());

		mat->myColor = { 0.8f, 0.8f, 0.8f };

		aiColor3D diffuse;
		if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse) == AI_SUCCESS)
		{
			mat->myColor = { diffuse.r, diffuse.g, diffuse.b };
		}

		mat->specular = 0.5f;

		myMaterials.push_back(mat);
	}
}

void Scene::ImportNode(const aiScene* aScene, const aiNode* aNode)
{
	aiMatrix4x4 tranform = aNode->mTransformation;

	const aiNode* at = aNode->mParent;
	while (at)
	{
		tranform *= at->mTransformation;
		at = at->mParent;
	}


	ImportMeshes(aScene->mMeshes, aNode, tranform);

	if (!aNode->mChildren)
		return;

	for (const aiNode* child : fisk::tools::RangeFromStartEnd(aNode->mChildren, aNode->mChildren + aNode->mNumChildren))
	{
		ImportNode(aScene, child);
	}
}

void Scene::ImportMeshes(aiMesh*const* aMeshes, const aiNode* aNode, const aiMatrix4x4& aTransform)
{
	if (!aNode->mMeshes)
		return;

	for (unsigned int meshIndex : fisk::tools::RangeFromStartEnd(aNode->mMeshes, aNode->mMeshes + aNode->mNumMeshes))
	{
		const aiMesh* mesh = aMeshes[meshIndex];

		if (!mesh->HasFaces())
			return;
		
		PolyObject obj = PolyObject::FromTri(TriFromFace(mesh->mVertices, mesh->mFaces[0], aTransform));

		for (const aiFace& face : fisk::tools::RangeFromStartEnd(mesh->mFaces + 1, mesh->mFaces + mesh->mNumFaces))
			obj.AddTri(TriFromFace(mesh->mVertices, face, aTransform));

		Add(obj, myMaterials[mesh->mMaterialIndex]);
	}
}

fisk::tools::Tri<float> Scene::TriFromFace(const aiVector3D* aVerticies, const aiFace& aFace, const aiMatrix4x4& aTransform)
{
	assert(aFace.mNumIndices == 3, "Mesh was not triangularized properly");

	return fisk::tools::Tri<float>::FromCorners(
		TranslateVectorType(aTransform * aVerticies[aFace.mIndices[0]]),
		TranslateVectorType(aTransform * aVerticies[aFace.mIndices[1]]),
		TranslateVectorType(aTransform * aVerticies[aFace.mIndices[2]]));
}

fisk::tools::V3f Scene::TranslateVectorType(const aiVector3D& aVector)
{
	return { aVector.x, aVector.y, aVector.z };
}
