

#include <windows.h>
#include <d3d11.h>
#include <thread>
#include <chrono>

#include "Camera.h"
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

#include "tools/Logger.h"

#include "imgui/imgui.h"

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

	constexpr size_t scaleFactor = 8;
	constexpr size_t samples = 1000;

	Camera::Lens lens;

	lens.myFocalLength = 20_m;
	lens.myDistance = 5_cm;
	lens.myRadius = 5_cm;
	lens.myFStop = 1/1.f;

	Camera camera(window.GetWindowSize() / scaleFactor, fisk::tools::Ray<float, 3>::FromPointandTarget({ 2_m, 6_m, 8_m }, { 0_m, 0_m, 0_m}), 100_deg, lens);

	Sky skyBox({ 1, 1, 1 }, 23_deg, { 1462.436f, 1049.146f, 303.80522f }, { 178.5f, 229.5f, 255.f });

	std::unique_ptr<Scene> scene = Scene::FromFile("C:/Users/Fi/Documents/Scenes/Example.fbx");

	DumbIntersector dumbIntersector(*scene);
	ClusteredIntersector clusterIntersector(*scene);


	std::vector<RayRenderer*> baseRenderers;
	std::vector<IAsyncRenderer<TextureType::PackedValues>*> renderers;

	for (size_t i = 0; i < 4; i++)
	{
		RayRenderer* baseRenderer = new RayRenderer(camera, dumbIntersector, skyBox, samples, i + 1);
		baseRenderers.push_back(baseRenderer);
		renderers.push_back(new ThreadedRenderer<TextureType::PackedValues, 1024>(*baseRenderer));
	}

	for (size_t i = 0; i < 4; i++)
	{
		RayRenderer* baseRenderer = new RayRenderer(camera, clusterIntersector, skyBox, samples, i + 1 + 100);
		baseRenderers.push_back(baseRenderer);
		renderers.push_back(new ThreadedRenderer<TextureType::PackedValues, 1024>(*baseRenderer));
	}

	TextureType texture(window.GetWindowSize() / scaleFactor, {});
	Orchestrator<TextureType> orcherstrator(texture, renderers);

	RaytracerOutputViewer viewer(framework, texture, window.GetWindowSize());
	
	fisk::tools::EventReg viewerImguiHandle = imguiHelper.DrawImgui.Register([&viewer](){ viewer.Imgui(); });
	fisk::tools::EventReg diagnosticImguiHandle = imguiHelper.DrawImgui.Register([](){ Diagnostics(); });

	fisk::tools::EventReg flushHandle = framework.AfterPresent.Register([&viewer]() { viewer.DrawImage(); });




	while (window.ProcessEvents())
	{

		fisk::tools::ScopeDiagnostic perfLock2("main Update");

		orcherstrator.Update(); // TODO: finish/save when done

		framework.Present(fisk::GraphicsFramework::VSyncState::Immediate);
	}
}