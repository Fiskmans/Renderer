#include "ClusteredIntersector.h"

#include <cassert>


namespace cluster_intersector
{

	Leaf::Leaf(const std::vector<fisk::tools::Tri<float>>& aFragments, Material* aMaterial, unsigned int aId)
	{
		assert(!aFragments.empty());

		myMaterial = aMaterial;
		myId = aId;

		myFragment = PolyObject::FromTri(aFragments[0]);

		for (size_t i = 1; i < aFragments.size(); i++)
			myFragment.AddTri(aFragments[i]);
	}

	void Leaf::Intersect(fisk::tools::Ray<float, 3> aRay, const fisk::tools::V3f& aPreDividedDirection, float& aInOutDepth, Hit& aInOutHit)
	{
		std::optional<float> boundingHit = fisk::tools::IntersectRayBoxPredivided<float, 3>(aRay.myOrigin, aPreDividedDirection, myFragment.myBoundingBox);

		if (!boundingHit)
			return;

		if (boundingHit >= aInOutDepth)
			return;

		for (size_t i = 0; i < myFragment.myTris.size(); i++)
		{
			const fisk::tools::Tri<float>& tri = myFragment.myTris[i];

			std::optional<float> hit = fisk::tools::Intersect(aRay, tri);

			if (!hit)
				continue;

			if (*hit > aInOutDepth)
				continue;

			aInOutDepth = *hit;

			fisk::tools::V3f pos = aRay.myOrigin + aRay.myDirection * *hit;

			aInOutHit.myPosition = pos;
			aInOutHit.myNormal = tri.Normal();
			aInOutHit.myObjectId = myId;
			aInOutHit.mySubObjectId = i + 1;
			aInOutHit.myMaterial = myMaterial;
		}
	}

	fisk::tools::AxisAlignedBox<float, 3> Leaf::GetBoundingBox()
	{
		return myFragment.myBoundingBox;
	}

	void Node::Intersect(fisk::tools::Ray<float, 3> aRay, const fisk::tools::V3f& aPreDividedDirection, float& aInOutDepth, Hit& aInOutHit)
	{
		std::optional<float> boundingHit = fisk::tools::IntersectRayBoxPredivided<float, 3>(aRay.myOrigin, aPreDividedDirection, myBoundingBox);

		if (!boundingHit)
			return;

		if (boundingHit >= aInOutDepth)
			return;

		for (std::unique_ptr<Leaf>& leaf : myLeafs)
			leaf->Intersect(aRay, aPreDividedDirection, aInOutDepth, aInOutHit);

		for (std::unique_ptr<Node>& node : myChildren)
			node->Intersect(aRay, aPreDividedDirection, aInOutDepth, aInOutHit);
	}

	void Node::Add(std::unique_ptr<Leaf>&& aLeaf)
	{
		if (myChildren.empty() && myLeafs.empty())
		{
			myBoundingBox = aLeaf->GetBoundingBox();
		}
		else
		{
			fisk::tools::AxisAlignedBox<float, 3> aabb = aLeaf->GetBoundingBox();
			myBoundingBox.ExpandToInclude(aabb.myMax);
			myBoundingBox.ExpandToInclude(aabb.myMin);
		}

		myLeafs.push_back(std::move(aLeaf));
	}

	void Node::Add(std::unique_ptr<Node>&& aNode)
	{
		if (myChildren.empty() && myLeafs.empty())
		{
			myBoundingBox = aNode->myBoundingBox;
		}
		else
		{
			myBoundingBox.ExpandToInclude(aNode->myBoundingBox.myMax);
			myBoundingBox.ExpandToInclude(aNode->myBoundingBox.myMin);
		}

		myChildren.push_back(std::move(aNode));
	}
}

ClusteredIntersector::ClusteredIntersector(const Scene& aScene)
{
	Bake(aScene);
}

std::optional<Hit> ClusteredIntersector::Intersect(fisk::tools::Ray<float, 3> aRay)
{
	fisk::tools::V3f preDivedRayDir
	{
		1.f / aRay.myDirection[0],
		1.f / aRay.myDirection[1],
		1.f / aRay.myDirection[2]
	};

	float depth = std::numeric_limits<float>::max();
	Hit hit;

	myRootNode->Intersect(aRay, preDivedRayDir, depth, hit);

	if (depth == std::numeric_limits<float>::max())
		return {};

	return hit;

}

void ClusteredIntersector::Bake(const Scene& aScene)
{
	myRootNode = std::make_unique<cluster_intersector::Node>();

	for (const SceneObject<PolyObject>& poly : aScene.myPolyObjects)
	{
		std::unique_ptr<cluster_intersector::Leaf> leaf = std::make_unique<cluster_intersector::Leaf>(poly.myShape.myTris, poly.myMaterial, poly.myId);

		myRootNode->Add(std::move(leaf));
	}
}