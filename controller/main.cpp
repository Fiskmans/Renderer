


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


void DiagnosticNode(fisk::tools::TimeTree* aTree)
{
	ImGui::Text("%s [%llu] %.3fs %2.1f%%", aTree->myName, aTree->myCallCount, aTree->myTime, aTree->myCovarage * 100.f);
	ImGui::Indent();

	for (fisk::tools::TimeTree* child : aTree->myChildren)
		DiagnosticNode(child);
	
	ImGui::Unindent();
}

void Diagnostics()
{
	ImGui::Begin("Diagnostics");

	DiagnosticNode(fisk::tools::GetTimeTreeRoot());

	ImGui::End();
}

int main(int argc, char** argv)
{
	using namespace std::chrono_literals;

	fisk::tools::ScopeDiagnostic perfLock1("main");

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

	RenderClient client(config, window.GetWindowSize() / scaleFactor, "C:/Users/Fi/Documents/Scenes/Example.fbx", std::make_shared<fisk::tools::TCPSocket>("104.154.30.17", "12345", 5s));

	std::vector<IAsyncRenderer<TextureType::PackedValues>*> renderers;
	std::optional<Orchestrator<TextureType>> orcherstrator;

	RaytracerOutputViewer viewer(framework, window.GetWindowSize(), window.GetWindowSize() / scaleFactor);

	viewer.AddTexture(client.GetTexture());

	fisk::tools::EventReg viewerImguiHandle = imguiHelper.DrawImgui.Register([&viewer]()
	{
		viewer.Imgui();
	});
	fisk::tools::EventReg diagnosticImguiHandle = imguiHelper.DrawImgui.Register([]()
	{
		Diagnostics();
	});
	
	/*
	fisk::tools::EventReg clusterImguiHandle = imguiHelper.DrawImgui.Register([&clusterIntersectorMedium, &window, &camera, scaleFactor]()
	{
		clusterIntersectorMedium.Imgui(window.GetWindowSize(), camera, scaleFactor);
	});*/

	fisk::tools::EventReg flushHandle = framework.AfterPresent.Register([&viewer]()
	{
		viewer.DrawImage();
	});

	while (window.ProcessEvents())
	{
		client.Update();

		fisk::tools::ScopeDiagnostic perfLock2("main Update");

		if(orcherstrator)
			orcherstrator->Update(); // TODO: finish/save when done

		framework.Present(fisk::GraphicsFramework::VSyncState::Immediate);
	}

	/*
	DumbIntersector dumbIntersector(*scene);
	ClusteredIntersector clusterIntersectorMedium(*scene, 8, 8);
	ClusteredIntersector clusterIntersectorFine(*scene, 2, 2);


	std::vector<RayRenderer*> baseRenderers;

	RayRenderer* baseA = new RayRenderer(camera, clusterIntersectorMedium, skyBox, samples, 1);
	baseRenderers.push_back(baseA);

	for (unsigned int i = 0; i < 6; i++)
	{
		ThreadedRenderer<TextureType::PackedValues, 1024>* threadA = new ThreadedRenderer<TextureType::PackedValues, 1024>(*baseA);

		renderers.push_back(threadA);
	}
	*/
}