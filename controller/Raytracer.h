#pragma once

#include "tools/MathVector.h"
#include "tools/Iterators.h"
#include "Shapes.h"

#include "GraphicsFramework.h"

#include "MultichannelTexture.h"
#include "IIntersector.h"
#include "Camera.h"
#include "Sky.h"

#include <vector>
#include <thread>
#include <mutex>
#include <memory>


class DoneException : public std::exception
{
	using std::exception::exception;
};

class Raytracer
{
public:
	using CompactNanoSecond = std::chrono::duration<float, std::nano>;
	using RayCaster = IRenderer<fisk::tools::Ray<float, 3>>;

	using TextureType = MultiChannelTexture<
		fisk::tools::V3f,	// Color
		CompactNanoSecond,	// Time taken
		unsigned int>;		// ObjectId>

	static constexpr size_t ColorChannel = 0;
	static constexpr size_t TimeChannel = 1;
	static constexpr size_t ObjectIdChannel = 2;

	Raytracer(fisk::tools::V2ui aSize, RayCaster& aRayCaster, IIntersector& aIntersector, Sky& aSky, float aAllowableNoise, unsigned int aPixelsPerTexel, size_t aWorkerThreads);
	~Raytracer();

	Raytracer(Raytracer&) = delete;
	Raytracer& operator=(Raytracer&) = delete;

	TextureType& GetTexture();

	void Imgui();


	static float ExpectedChange(fisk::tools::V3f aVarianceAggregate, unsigned int aSamples);

private:

	enum class RayState : uint8_t
	{
		Empty,
		Working,
		Done
	};

	struct RayJob
	{
		fisk::tools::V2ui myUV;
		TextureType::PackedValues myResult;

		std::atomic<RayState> myState;
	};

	class WorkerThread : public IAsyncRenderer<TextureType::PackedValues>
	{
	public:
		constexpr static size_t SampleQueueSize = 16;
		constexpr static size_t MaxBounces = 16;
		
		WorkerThread(RayCaster& aRaycaster, IIntersector& aIntersector, Sky& aSky, size_t aSamplesPerTexel);
		~WorkerThread();

		WorkerThread(WorkerThread&) = delete;
		WorkerThread& operator=(WorkerThread&) = delete;
		WorkerThread(WorkerThread&&) = delete;
		WorkerThread& operator=(WorkerThread&&) = delete;

		void StopThread();

		void Imgui();

		bool CanRender() override;

		void Render(fisk::tools::V2ui aUV) override;
		bool GetResult(Result& aOut) override;

		size_t GetPending() override;

	private:

		struct Sample
		{
			fisk::tools::V3f myColor{ 1, 1, 1 };
			unsigned int myObjectId = 0;
		};

		void Run();

		TextureType::PackedValues RenderTexel(fisk::tools::V2ui aUV); // TODO: Extract this into a IRenderer instead of baking it into the thread construct
		Sample SampleTexel(fisk::tools::V2ui aUV);

		RayCaster& myRayCaster;			// Shared
		IIntersector& myIntersector;	// Shared
		Sky& mySky;						// Shared
		size_t mySamplesPerTexel;		// Shared


		std::thread myThread;	// External thread
		size_t myPending;

		RayJob myRays[SampleQueueSize]; // Shared

		fisk::tools::LoopingPointer<RayJob> myWriteHead; // External thread
		fisk::tools::LoopingPointer<RayJob> myReadHead; // External thread

		fisk::tools::LoopingPointer<RayJob> myRenderHead; // Internal thread

		std::atomic<bool> myStopRequested; // Shared
	};

	void Setup();

	void Run();

	IRenderer<fisk::tools::Ray<float, 3>>& myRayCaster;
	IIntersector& myIntersector;
	Sky& mySky;
	float myAllowableNoise;

	unsigned int myPixelsPerTexel;

	TextureType myTextureData;
	fisk::tools::V2ui mySize;
	
	WorkerThread* myThreads; // threads non-movability means its easier to use c-style arrays
	size_t mythreadCount;

	std::thread myOrchistratorThread;
	std::recursive_mutex myOrchistratorMutex;
	bool myIsDone;
	std::atomic<bool> myStopRequested;
};

