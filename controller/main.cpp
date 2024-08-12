


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


void DiagnosticNode(const fisk::tools::Trace& aTrace)
{
	ImGui::TableNextRow();

	ImGui::TableNextColumn();
	ImGui::Text("%s", aTrace.myTag.c_str());

	ImGui::TableNextColumn();
	ImGui::Text("%zu", aTrace.myTimesTraced);

	ImGui::TableNextColumn();
	ImGui::Text("%.3fs", std::chrono::duration_cast<std::chrono::duration<float>>(aTrace.myTimeSpent).count());

	for (const fisk::tools::Trace& child : aTrace.myChildren)
		DiagnosticNode(child);
}

void Diagnostics()
{
	ImGui::Begin("Diagnostics");
	
	if (ImGui::BeginTable("diag_table", 4))
	{
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Calls");
		ImGui::TableSetupColumn("Time");
		ImGui::TableHeadersRow();

		for (const fisk::tools::Trace& trace : fisk::tools::PerformanceTracer::GetRoots())
		{
			DiagnosticNode(trace);
		}

		ImGui::EndTable();
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