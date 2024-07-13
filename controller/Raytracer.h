#pragma once

#include "tools/MathVector.h"
#include "Shapes.h"

#include "GraphicsFramework.h"

#include "MultichannelTexture.h"
#include "IIntersector.h"
#include "RegionJob.h"
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

	using TextureType = MultiChannelTexture<
		fisk::tools::V3f,	// Color
		CompactNanoSecond,	// Time taken
		unsigned int,		// ObjectId
		unsigned int,		// Samples
		fisk::tools::V3f>;	// VarianceAggregate

	static constexpr size_t ColorChannel = 0;
	static constexpr size_t TimeChannel = 1;
	static constexpr size_t ObjectIdChannel = 2;
	static constexpr size_t SamplesChannel = 3;
	static constexpr size_t VarianceAggregateChannel = 4;

	Raytracer(fisk::tools::V2ui aSize, Camera& aCamera, IIntersector& aIntersector, Sky& aSky, float aAllowableNoise, unsigned int aPixelsPerTexel, size_t aWorkerThreads);
	~Raytracer();

	Raytracer(Raytracer&) = delete;
	Raytracer& operator=(Raytracer&) = delete;

	TextureType& GetTexture();

	void Imgui();


	static float ExpectedChange(fisk::tools::V3f aVarianceAggregate, unsigned int aSamples);

private:

	struct RayJob
	{
		fisk::tools::Ray<float, 3> myRay;
		unsigned int myFirstObject;
		unsigned int myBounces;

		fisk::tools::V3f myColor;
		fisk::tools::V2ui myOriginTexel;

		CompactNanoSecond myTimeSpent;
	};

	class WorkerThread
	{
	public:
		constexpr static size_t SampleQueueSize = 512;
		constexpr static size_t SampleEnqueueBatchSize = SampleQueueSize / 2;
		
		WorkerThread(IIntersector& aIntersector, Sky& aSky);
		~WorkerThread();

		WorkerThread(WorkerThread&) = delete;
		WorkerThread& operator=(WorkerThread&) = delete;
		WorkerThread(WorkerThread&&) = delete;
		WorkerThread& operator=(WorkerThread&&) = delete;

		void StopThread();

		void Imgui();

		bool TryQueue(RayJob aJob);
		bool TryRetrieve(RayJob& aOutJob);

		bool IsWorking();

	private:

		void Run();

		bool StepRay(std::atomic<RayJob>& aJob);

		bool CalculateNextRay(RayJob& aInOutJob);
		
		enum class State : uint8_t
		{
			Empty,
			Working,
			Done
		};

		IIntersector& myIntersector; // Shared
		Sky& mySky; // Shared

		size_t myWriteHead; // External thread
		size_t myReadHead; // External thread

		std::thread myThread; // External thread

		std::atomic<RayJob> myRays[SampleQueueSize]; // Shared
		std::atomic<State> myStates[SampleQueueSize]; // Shared

		std::atomic<bool> myStopRequested; // Shared
	};

	void Setup();

	void Run();

	void QueueRegion(const RegionJob& aJob);

	RayJob CurrentRay();
	void EnqueueTexels();
	bool NextSample();

	void FindMoreWork();
	void QueueTexels(const std::vector<fisk::tools::V2ui>& aTexels, size_t aSamplesPerPixel, size_t aExpansion, std::string aName);

	void MergeOuput();

	Camera& myCamera;
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
	std::vector<RegionJob> myRegionJobs;
	bool myIsDone;
	std::atomic<bool> myStopRequested;
};

