


#include "Camera.h"
#include "CheckeredRenderer.h"
#include "GraphicsFramework.h"
#include "ImguiHelper.h"
#include "Orchestrator.h"
#include "RayRenderer.h"
#include "RaytracerOutputViewer.h"
#include "Scene.h"
#include "ThreadedRenderer.h"
#include "Window.h"
#include "intersectors/DumbIntersector.h"
#include "intersectors/ClusteredIntersector.h"
#include "Protocol.h"

#include "NetworkedRenderer.h"
#include "RenderClient.h"
#include "RenderConfig.h"

#include "tools/Logger.h"

#include "imgui/imgui.h"

#include <iostream>
#include <d3d11.h>
#include <thread>
#include <chrono>
#include <concepts>

namespace imgui_utils
{
	template<class NodeType, class DrawingFunction, class ChildGetter>
		requires 
		requires(NodeType& node, DrawingFunction&& drawing, ChildGetter&& childGetter)
		{
			{ drawing(node) };
			{ *std::begin(childGetter(node)) } -> std::convertible_to<NodeType&>;
		}
	ImVec2 Tree(NodeType& aNode, DrawingFunction&& aDrawingFunction, ChildGetter&& aChildGetter)
	{
		ImVec2 anchor;
		ImVec2 treeRoot;

		{

			ImVec2 start = ImGui::GetCursorScreenPos();
			ImGui::BeginChild("node", { 0, 0 }, ImGuiChildFlags_AutoResizeY);

			aDrawingFunction(aNode);

			ImGui::EndChild();
			ImVec2 end = ImGui::GetCursorScreenPos();

			anchor = {
				start.x,
				(start.y + end.y) / 2.f
			};

			treeRoot = {
				end.x + 5.f,
				end.y
			};
		}

		ImGui::Indent(15.f);
		std::vector<ImVec2> anchors;
		for (NodeType& child : aChildGetter(aNode))
		{
			ImGui::PushID(static_cast<int>(anchors.size()));

			anchors.push_back(Tree(child, aDrawingFunction, aChildGetter));

			ImGui::PopID();
		}
		ImGui::Unindent(15.f);

		ImDrawList* drawlist = ImGui::GetWindowDrawList();

		ImColor color = ImGui::GetStyle().Colors[ImGuiCol_Border];
		
		for (ImVec2& anchor : anchors)
		{
			ImVec2 next = {
				treeRoot.x,
				anchor.y
			};

			drawlist->AddLine(treeRoot, next, color);
			drawlist->AddLine(next, anchor, color);

			treeRoot = next;
		}

		return anchor;
	}
}

void DiagnosticNode(const fisk::tools::Trace& aTrace)
{
	ImGui::Text("%s [%zu]", aTrace.myTag.c_str(), aTrace.myTimesTraced);

	ImGui::SameLine();
	ImGui::Text("%.3fs", std::chrono::duration_cast<std::chrono::duration<float>>(aTrace.myTimeSpent).count());
}

void Diagnostics()
{
	ImGui::Begin("Diagnostics");
	
	for (const fisk::tools::Trace& trace : fisk::tools::PerformanceTracer::GetRoots())
	{
		imgui_utils::Tree(trace, DiagnosticNode, [](const fisk::tools::Trace& aTrace)
		{
			return aTrace.myChildren;
		});
	}

	ImGui::ShowDemoWindow();
	ImGui::End();
}

int main(int argc, char** argv)
{
	using namespace std::chrono_literals;


	fisk::tools::SetFilter(fisk::tools::All);
	
	fisk::tools::SetColor(fisk::tools::AnyWarning, FOREGROUND_RED | FOREGROUND_GREEN);
	fisk::tools::SetColor(fisk::tools::AnyError, FOREGROUND_RED);

	fisk::tools::SetHalting(fisk::tools::AnyError);

	fisk::Window window;

	fisk::GraphicsFramework framework(window);

	fisk::ImguiHelper imguiHelper(framework, window);

	constexpr size_t scaleFactor = 4;
	constexpr size_t samples = 400;

	RenderConfig config;

	config.myMode = RenderConfig::RaytracedClustered;
	config.myRenderId = 1;
	config.mySamplesPerTexel = samples;

	std::string scene = "../../scenes/Example.fbx";

	RenderClient client(config, window.GetWindowSize() / scaleFactor, scene, std::make_shared<fisk::tools::TCPSocket>("104.154.30.17", "11587", 5s));


	RaytracerOutputViewer viewer(framework, window.GetWindowSize(), window.GetWindowSize() / scaleFactor);

	viewer.AddTexture(client.GetTexture());

	fisk::tools::EventReg viewerImguiHandle = imguiHelper.DrawImgui.Register([&viewer]() { viewer.Imgui(); });
	fisk::tools::EventReg diagnosticImguiHandle = imguiHelper.DrawImgui.Register([]() { Diagnostics(); });
	fisk::tools::EventReg flushHandle = framework.AfterPresent.Register([&viewer]() { viewer.DrawImage(); });

	while (true)
	{
		FISK_TRACE("main loop");

		if (!window.ProcessEvents())
			break;

		client.Update();

		framework.Present(fisk::GraphicsFramework::VSyncState::Immediate);
	}
}