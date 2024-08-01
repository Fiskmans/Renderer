#pragma once

#include "tools/Shapes.h"
#include "Scene.h"
#include "IIntersector.h"
#include "PolyObject.h"
#include "Material.h"
#include "Camera.h"

#include <string>
#include <vector>
#include <optional>
#include <utility>

namespace cluster_intersector
{
	struct NamedBoundingBox
	{
		fisk::tools::AxisAlignedBox<float, 3> myBox;
		std::string myName;
		bool myIsHighlighted;
		std::vector<fisk::tools::Tri<float>> myTris;
	};

	class Leaf
	{
	public:
		Leaf(const std::vector<fisk::tools::Tri<float>>& aFragments, const Material* aMaterial, unsigned int aId, std::string aName);

		void Intersect(fisk::tools::Ray<float, 3> aRay,const fisk::tools::V3f& aPreDividedDirection, float& aInOutDepth, Hit& aInOutHit);

		fisk::tools::AxisAlignedBox<float, 3> GetBoundingBox();

		void Imgui();

		void CollectBoxes(std::vector<NamedBoundingBox>& aBoxSink, fisk::tools::Ray<float, 3> aRay);

	private:
		std::string myName;

		unsigned int myId;
		PolyObject myFragment;
		const Material* myMaterial;
	};

	class Node
	{
	public:
		Node(std::string aName);

		void Intersect(fisk::tools::Ray<float, 3> aRay,const fisk::tools::V3f& aPreDividedDirection, float& aInOutDepth, Hit& aInOutHit);

		void Add(std::unique_ptr<Leaf>&& aLeaf);
		void Add(std::unique_ptr<Node>&& aNode);

		void Imgui();

		void CollectBoxes(std::vector<NamedBoundingBox>& aBoxSink, fisk::tools::Ray<float, 3> aRay);

		fisk::tools::AxisAlignedBox<float, 3> GetBoundingBox();
		
	private:
		std::string myName;

		bool myWasOpen;
		fisk::tools::AxisAlignedBox<float, 3> myBoundingBox;
		std::vector<std::unique_ptr<Node>> myChildren;
		std::vector<std::unique_ptr<Leaf>> myLeafs;
	};
}

class ClusteredIntersector : public IIntersector
{
public:
	ClusteredIntersector(const Scene& aScene, size_t aFragmentSize, size_t aClustersPerNode);

	std::optional<Hit> Intersect(fisk::tools::Ray<float, 3> aRay) override;

	void Imgui(fisk::tools::V2ui aWindowSize, Camera& aCamera, size_t aRenderScale);

private:
	void Bake(const Scene& aScene, size_t aFragmentSize, size_t aClustersPerNode);

	std::unique_ptr<cluster_intersector::Node> myRootNode;
};

