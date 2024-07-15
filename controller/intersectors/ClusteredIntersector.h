#pragma once

#include "tools/Shapes.h"
#include "Scene.h"
#include "IIntersector.h"
#include "PolyObject.h"
#include "Material.h"

#include <vector>
#include <optional>
#include <utility>

namespace cluster_intersector
{
	class Leaf
	{
	public:
		Leaf(const std::vector<fisk::tools::Tri<float>>& aFragments, Material* aMaterial, unsigned int aId);

		void Intersect(fisk::tools::Ray<float, 3> aRay,const fisk::tools::V3f& aPreDividedDirection, float& aInOutDepth, Hit& aInOutHit);

		fisk::tools::AxisAlignedBox<float, 3> GetBoundingBox();

	private:
		unsigned int myId;
		PolyObject myFragment;
		Material* myMaterial;
	};

	class Node
	{
	public:
		void Intersect(fisk::tools::Ray<float, 3> aRay,const fisk::tools::V3f& aPreDividedDirection, float& aInOutDepth, Hit& aInOutHit);

		void Add(std::unique_ptr<Leaf>&& aLeaf);
		void Add(std::unique_ptr<Node>&& aNode);
		
	private:
		fisk::tools::AxisAlignedBox<float, 3> myBoundingBox;
		std::vector<std::unique_ptr<Node>> myChildren;
		std::vector<std::unique_ptr<Leaf>> myLeafs;
	};
}

class ClusteredIntersector : public IIntersector
{
public:
	ClusteredIntersector(const Scene& aScene);

	std::optional<Hit> Intersect(fisk::tools::Ray<float, 3> aRay) override;

private:
	void Bake(const Scene& aScene);

	std::unique_ptr<cluster_intersector::Node> myRootNode;
};

