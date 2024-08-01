#include "ClusteredIntersector.h"

#include "imgui/imgui.h"

#include <cassert>
#include <random>


namespace cluster_intersector
{

	Leaf::Leaf(const std::vector<fisk::tools::Tri<float>>& aFragments, const Material* aMaterial, unsigned int aId, std::string aName)
	{
		assert(!aFragments.empty());

		myMaterial = aMaterial;
		myId = aId;
		myName = aName;

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

		for (unsigned int i = 0; i < myFragment.myTris.size(); i++)
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

	void Leaf::Imgui()
	{
		if (ImGui::TreeNode(myName.c_str()))
		{
			ImGui::TreePop();
		}
	}

	void Leaf::CollectBoxes(std::vector<NamedBoundingBox>& aBoxSink, fisk::tools::Ray<float, 3> aRay)
	{
		NamedBoundingBox box;
		box.myBox = myFragment.myBoundingBox;
		box.myIsHighlighted = !!fisk::tools::Intersect(aRay, myFragment.myBoundingBox);
		box.myName = myName;

		if (box.myIsHighlighted)
		{
			box.myTris = myFragment.myTris;
		}

		aBoxSink.push_back(box);
	}

	Node::Node(std::string aName)
	{
		myWasOpen = false;
		myName = aName;
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

	void Node::Imgui()
	{
		myWasOpen = false;

		if (ImGui::TreeNode(myName.c_str()))
		{
			myWasOpen = true;
			if (!myLeafs.empty())
			{
				for (std::unique_ptr<Leaf>& leaf : myLeafs)
					leaf->Imgui();

				ImGui::Separator();
			}
			if (!myChildren.empty())
			{
				for (std::unique_ptr<Node>& node : myChildren)
					node->Imgui();
			}
			ImGui::TreePop();
		}
	}
	void Node::CollectBoxes(std::vector<NamedBoundingBox>& aBoxSink, fisk::tools::Ray<float, 3> aRay)
	{
		if (!myWasOpen && !fisk::tools::Intersect(aRay, myBoundingBox))
			return;

		NamedBoundingBox box;

		box.myBox = myBoundingBox;
		box.myName = myName;

		aBoxSink.push_back(box);

		for (std::unique_ptr<Node>& child : myChildren)
		{
			child->CollectBoxes(aBoxSink, aRay);
		}

		for (std::unique_ptr<Leaf>& leaf : myLeafs)
		{
			leaf->CollectBoxes(aBoxSink, aRay);
		}
	}

	fisk::tools::AxisAlignedBox<float, 3> Node::GetBoundingBox()
	{
		return myBoundingBox;
	}
}

ClusteredIntersector::ClusteredIntersector(const Scene& aScene, size_t aFragmentSize, size_t aClustersPerNode)
{
	Bake(aScene, aFragmentSize, aClustersPerNode);
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

void ClusteredIntersector::Imgui(fisk::tools::V2ui aWindowSize, Camera& aCamera, size_t aRenderScale)
{
	float floatRenderScale = static_cast<float>(aRenderScale);

	ImVec2 cursorPos = ImGui::GetMousePos();

	cursorPos.x = std::min(cursorPos.x, static_cast<float>(aWindowSize[0]));
	cursorPos.y = std::min(cursorPos.y, static_cast<float>(aWindowSize[1]));

	cursorPos.x = std::max(cursorPos.x, 0.f);
	cursorPos.y = std::max(cursorPos.y, 0.f);

	fisk::tools::Ray<float, 3> mouseRay = aCamera.Render({ static_cast<unsigned int>(cursorPos.x / floatRenderScale), static_cast<unsigned int>(cursorPos.y / floatRenderScale) });
	std::vector<cluster_intersector::NamedBoundingBox> boxes;

	ImGui::Begin("Clustered Intersector");

	myRootNode->Imgui();
	myRootNode->CollectBoxes(boxes, mouseRay);

	ImGui::End();

	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ static_cast<float>(aWindowSize[0]), static_cast<float>(aWindowSize[1]) });
	ImGui::Begin("Box overlay", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs);

	ImDrawList* drawList = ImGui::GetWindowDrawList();


	bool enableHover = !ImGui::GetIO().WantCaptureMouse;

	for (cluster_intersector::NamedBoundingBox& box : boxes)
	{
		ImColor color = ImColor(200, 120, 30, 100);
		ImColor fullColor = ImColor(200, 120, 30, 255);

		if (box.myIsHighlighted)
		{
			color = ImColor(100, 230, 20, 100);
			fullColor = ImColor(100, 230, 20, 255);
		}

		if (enableHover)
		{
			if (fisk::tools::Intersect(mouseRay, box.myBox))
			{
				color = ImColor(180, 190, 230, 100);
				fullColor = ImColor(180, 190, 230, 255);
			}
		}

		for (fisk::tools::Tri<float>& tri : box.myTris)
		{
			std::optional<fisk::tools::V2f> a = aCamera.GetScreenPos(tri.myOrigin);
			std::optional<fisk::tools::V2f> b = aCamera.GetScreenPos(tri.myOrigin + tri.mySideA);
			std::optional<fisk::tools::V2f> c = aCamera.GetScreenPos(tri.myOrigin + tri.mySideB);
			if (a && b)
			{
				fisk::tools::V2f from = *a * floatRenderScale;
				fisk::tools::V2f to = *b * floatRenderScale;
				drawList->AddLine({ from[0], from[1] }, { to[0], to[1] }, fullColor);
			}
			if (b && c)
			{
				fisk::tools::V2f from = *b * floatRenderScale;
				fisk::tools::V2f to = *c * floatRenderScale;
				drawList->AddLine({ from[0], from[1] }, { to[0], to[1] }, fullColor);
			}
			if (c && a)
			{
				fisk::tools::V2f from = *c * floatRenderScale;
				fisk::tools::V2f to = *a * floatRenderScale;
				drawList->AddLine({ from[0], from[1] }, { to[0], to[1] }, fullColor);
			}
		}

		fisk::tools::V3f worldCorners[8] = {
			{ box.myBox.myMax[0], box.myBox.myMax[1], box.myBox.myMax[2] },
			{ box.myBox.myMax[0], box.myBox.myMax[1], box.myBox.myMin[2] },
			{ box.myBox.myMax[0], box.myBox.myMin[1], box.myBox.myMax[2] },
			{ box.myBox.myMax[0], box.myBox.myMin[1], box.myBox.myMin[2] },
			{ box.myBox.myMin[0], box.myBox.myMax[1], box.myBox.myMax[2] },
			{ box.myBox.myMin[0], box.myBox.myMax[1], box.myBox.myMin[2] },
			{ box.myBox.myMin[0], box.myBox.myMin[1], box.myBox.myMax[2] },
			{ box.myBox.myMin[0], box.myBox.myMin[1], box.myBox.myMin[2] }
		};

		std::optional<fisk::tools::V2f> screenCorners[8] = {
			aCamera.GetScreenPos(worldCorners[0]),
			aCamera.GetScreenPos(worldCorners[1]),
			aCamera.GetScreenPos(worldCorners[2]),
			aCamera.GetScreenPos(worldCorners[3]),
			aCamera.GetScreenPos(worldCorners[4]),
			aCamera.GetScreenPos(worldCorners[5]),
			aCamera.GetScreenPos(worldCorners[6]),
			aCamera.GetScreenPos(worldCorners[7])
		};

		for (std::pair<size_t, size_t>& pairing : std::vector<std::pair<size_t, size_t>>{
			{ 0, 1 },
			{ 0, 2 },
			{ 0, 4 },
			{ 1, 3 },
			{ 1, 5 },
			{ 2, 3 },
			{ 2, 6 },
			{ 3, 7 },
			{ 4, 5 },
			{ 4, 6 },
			{ 5, 7 },
			{ 6, 7 }
			})
		{
			if (screenCorners[pairing.first] && screenCorners[pairing.second])
			{
				fisk::tools::V2f from = *screenCorners[pairing.first] * floatRenderScale;
				fisk::tools::V2f to = *screenCorners[pairing.second] * floatRenderScale;

				drawList->AddLine({ from[0], from[1] }, { to[0], to[1] }, color);
			}
		}

		if (screenCorners[0])
		{
			fisk::tools::V2f point = *screenCorners[0] * floatRenderScale;
			drawList->AddText({ point[0], point[1] }, color, box.myName.c_str());
		}
	}


	ImGui::End();
}

void ClusteredIntersector::Bake(const Scene& aScene, size_t aFragmentSize, size_t aClustersPerNode)
{
	std::random_device seed;
	std::mt19937 rng(seed());

	myRootNode = std::make_unique<cluster_intersector::Node>("Root");

	size_t leafIndex = 0;
	size_t nodeIndex = 0;

	for (const SceneObject<PolyObject>& poly : aScene.myPolyObjects)
	{
		std::vector<std::unique_ptr<cluster_intersector::Leaf>> leafs;
		std::vector<std::unique_ptr<cluster_intersector::Node>> nodes;

		// Generate leafs
		{
			std::vector<std::vector<fisk::tools::Tri<float>>> fragments;
			std::vector<fisk::tools::Tri<float>> allTris = poly.myShape.myTris;

			assert(!allTris.empty());

			std::shuffle(allTris.begin(), allTris.end(), rng);


			fragments.resize(allTris.size() / aFragmentSize + 1);
		
			// Initial seeds
			for (size_t i = 0; i < fragments.size(); i++)
				fragments[i].push_back(allTris[i]);

			// group tris by the closest seed
			for (size_t i = fragments.size(); i < allTris.size(); i++)
			{
				fisk::tools::Tri<float>& tri = allTris[i];

				decltype(fragments)::iterator bestFragment = std::min_element(fragments.begin(), fragments.end(),
					[&tri](const std::vector<fisk::tools::Tri<float>>& aLeftFragment, const std::vector<fisk::tools::Tri<float>>& aRightFragment)
					{
						fisk::tools::V3f leftCenter = aLeftFragment[0].myOrigin + (aLeftFragment[0].mySideA + aLeftFragment[0].mySideB) / 3.f;
						fisk::tools::V3f rightCenter = aRightFragment[0].myOrigin + (aRightFragment[0].mySideA + aRightFragment[0].mySideB) / 3.f;
						fisk::tools::V3f triCenter = tri.myOrigin + (tri.mySideA + tri.mySideB) / 3.f;

						return leftCenter.Distance(triCenter) < rightCenter.Distance(triCenter);
					});

				bestFragment->push_back(tri);
			}

			leafs.reserve(fragments.size());

			for (std::vector<fisk::tools::Tri<float>>& fragment : fragments)
			{
				leafs.emplace_back(std::make_unique<cluster_intersector::Leaf>(fragment, aScene.GetMaterial(poly.myMaterialIndex), poly.myId, "Leaf: " + std::to_string(leafIndex++)));
			}
		}

		//Generate initial leaf clusters
		{
			std::shuffle(leafs.begin(), leafs.end(), rng);

			nodes.reserve((leafs.size() / aClustersPerNode) + 1);

			// seed clusters
			for (size_t i = 0; i < (leafs.size() / aClustersPerNode) + 1; i++)
			{
				std::unique_ptr<cluster_intersector::Node> node = std::make_unique<cluster_intersector::Node>("Node: " +std::to_string(nodeIndex++));

				node->Add(std::move(leafs[i]));

				nodes.push_back(std::move(node));
			}

			// group clusters by the closest node, yes the nodes are modified as we loop ¯\_(*-*)_/¯
			for (size_t i = nodes.size(); i < leafs.size(); i++)
			{
				std::unique_ptr<cluster_intersector::Leaf>& leaf = leafs[i];

				decltype(nodes)::iterator bestNode = std::min_element(nodes.begin(), nodes.end(),
					[&leaf](const std::unique_ptr<cluster_intersector::Node>& aLeftFragment, const std::unique_ptr<cluster_intersector::Node>& aRightFragment)
				{
					fisk::tools::V3f leftCenter = (aLeftFragment->GetBoundingBox().myMin + aLeftFragment->GetBoundingBox().myMax) / 2.f;
					fisk::tools::V3f rightCenter = (aRightFragment->GetBoundingBox().myMin + aRightFragment->GetBoundingBox().myMax) / 2.f;
					fisk::tools::V3f triCenter = (leaf->GetBoundingBox().myMin + leaf->GetBoundingBox().myMax) / 2.f;

					return leftCenter.Distance(triCenter) < rightCenter.Distance(triCenter);
				});

				(*bestNode)->Add(std::move(leaf));
			}
		}

		while (nodes.size() > aClustersPerNode)
		{
			std::vector<std::unique_ptr<cluster_intersector::Node>> clusters;

			std::shuffle(nodes.begin(), nodes.end(), rng);

			clusters.reserve((nodes.size() / aClustersPerNode) + 1);

			// seed clusters
			for (size_t i = 0; i < (nodes.size() / aClustersPerNode) + 1; i++)
			{
				std::unique_ptr<cluster_intersector::Node> node = std::make_unique<cluster_intersector::Node>("Node: " + std::to_string(nodeIndex++));

				node->Add(std::move(nodes[i]));

				clusters.push_back(std::move(node));
			}

			// group clusters by the closest node, yes the nodes are modified as we loop ¯\_(*-*)_/¯
			for (size_t i = clusters.size(); i < nodes.size(); i++)
			{
				std::unique_ptr<cluster_intersector::Node>& node = nodes[i];

				decltype(clusters)::iterator bestNode = std::min_element(clusters.begin(), clusters.end(),
					[&node](const std::unique_ptr<cluster_intersector::Node>& aLeftFragment, const std::unique_ptr<cluster_intersector::Node>& aRightFragment)
				{
					fisk::tools::V3f leftCenter = (aLeftFragment->GetBoundingBox().myMin + aLeftFragment->GetBoundingBox().myMax) / 2.f;
					fisk::tools::V3f rightCenter = (aRightFragment->GetBoundingBox().myMin + aRightFragment->GetBoundingBox().myMax) / 2.f;
					fisk::tools::V3f triCenter = (node->GetBoundingBox().myMin + node->GetBoundingBox().myMax) / 2.f;

					return leftCenter.Distance(triCenter) < rightCenter.Distance(triCenter);
				});

				(*bestNode)->Add(std::move(node));
			}

			nodes = std::move(clusters);
		}

		// TODO: build a node tree
		for (std::unique_ptr<cluster_intersector::Node>& node : nodes)
			myRootNode->Add(std::move(node));
	}
}