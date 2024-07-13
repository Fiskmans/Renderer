

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

#include "tools/Logger.h"

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

	constexpr size_t scaleFactor = 8;

	Camera::Lens lens;

	lens.myFocalLength = 20_m;
	lens.myDistance = 5_cm;
	lens.myRadius = 5_cm;
	lens.myFStop = 1/1.f;

	Camera camera(window.GetWindowSize() / scaleFactor, fisk::tools::Ray<float, 3>::FromPointandTarget({ 2_m, 6_m, 8_m }, { 0_m, 0_m, 0_m}), 100_deg, lens);

	Sky skyBox({ 1, 1, 1 }, 20_deg, { 1462.436f, 1049.146f, 303.80522f }, { 178.5f, 229.5f, 255.f });

	Scene scene;


	Material redMat;
	Material greenMat;
	Material blueMat;
	Material grayMat;
	Material leftWallMat;
	Material frontWallMat;

	{
		redMat.myColor = { 1, 0.3f, 0.3f };
		redMat.specular = 0.2f;
	}
	{
		greenMat.myColor = { 0.3f, 1, 0.3f };
		greenMat.specular = 0.1f;
	}
	{
		blueMat.myColor = { 0.3f, 0.3f, 1 };
		blueMat.specular = 0.4f;
	}
	{
		grayMat.myColor = { 0.4f, 0.4f, 0.4f };
		grayMat.specular = 0.01f;
	}
	{
		leftWallMat.myColor = { 0.9f, 0.86f, 0.8f };
		leftWallMat.specular = 0.1f;
	}
	{
		leftWallMat.myColor = { 0.96f, 0.86f, 0.8f };
		leftWallMat.specular = 0.01f;
	}

	{
		fisk::tools::Sphere<float> sphere;

		sphere.myCenter = { 2_m, 1_m, 1_m };
		sphere.myRadius = 1_m;

		scene.Add(sphere, &redMat);
	}
	{
		fisk::tools::Sphere<float> sphere;

		sphere.myCenter = { 6_m, 3_m, 1_m };
		sphere.myRadius = 3_m;

		scene.Add(sphere, &greenMat);
	}
	{
		fisk::tools::Sphere<float> sphere;

		sphere.myCenter = { -3_m, 2_m, -1_m };
		sphere.myRadius = 2_m;

		scene.Add(sphere, &blueMat);
	}

	{
		fisk::tools::Plane<float> plane;

		plane.myNormal = { 0, 1, 0 };
		plane.myDistance = 0_m;

		scene.Add(plane, &grayMat);
	}
	//{
	//	fisk::tools::Plane<float> plane;
	//
	//	plane.myNormal = { 0, 0, 1 };
	//	plane.myDistance = -6_m;
	//
	//	scene.Add(plane, &leftWallMat);
	//}
	//{
	//	fisk::tools::Plane<float> plane;
	//
	//	plane.myNormal = { 1, 0, 0 };
	//	plane.myDistance = -5_m - 5_cm;
	//
	//	scene.Add(plane, &frontWallMat);
	//}

	DumbIntersector dumbIntersector(scene);

	RayRenderer baseRenderer(camera, dumbIntersector, skyBox, 1000);

	std::vector<IAsyncRenderer<TextureType::PackedValues>*> renderers;

	for (size_t i = 0; i < (std::max)(static_cast<int>(std::thread::hardware_concurrency()) - 2, 4); i++)
	{
		renderers.push_back(new ThreadedRenderer<TextureType::PackedValues, 16>(baseRenderer));
	}

	TextureType texture(window.GetWindowSize() / scaleFactor, {});
	Orchestrator<TextureType> orcherstrator(texture, renderers);

	RaytracerOutputViewer viewer(framework, texture, window.GetWindowSize());
	
	fisk::tools::EventReg viewerImguiHandle = imguiHelper.DrawImgui.Register([&viewer](){ viewer.Imgui(); });
	fisk::tools::EventReg flushHandle = framework.AfterPresent.Register([&viewer](){ viewer.DrawImage(); });

	while (window.ProcessEvents())
	{
		orcherstrator.Update(); // TODO: finnish
		framework.Present(fisk::GraphicsFramework::VSyncState::OnVerticalBlank);
	}
}