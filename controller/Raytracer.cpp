#include "Raytracer.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "tools/Iterators.h"
#include "ConvertVector.h"
#include "PartitionBy.h"
#include "RegionGenerator.h"

#include <random>

template<class MutexType>
class TimedLockGuard
{
public:
	TimedLockGuard(MutexType& aMutex, std::chrono::microseconds aTimeout)
		: myMutex(aMutex)
	{
		using clock = std::chrono::steady_clock;

		clock::time_point start = clock::now();

		while (clock::now() - start < aTimeout)
		{
			if (myMutex.try_lock())
			{
				myHasLock = true;
				return;
			}
		}
	}

	~TimedLockGuard()
	{
		if (myHasLock)
			myMutex.unlock();
	}

	explicit operator bool()
	{
		return myHasLock;
	}

private:
	MutexType& myMutex;
	bool myHasLock = false;
};

Raytracer::Raytracer(fisk::tools::V2ui aSize, IRenderer<fisk::tools::Ray<float, 3>>& aRayCaster, IIntersector& aIntersector, Sky& aSky, float aAllowableNoise, unsigned int aPixelsPerTexel, size_t aWorkerThreads)
	: myRayCaster(aRayCaster)
	, myIntersector(aIntersector)
	, mySky(aSky)
	, myAllowableNoise(aAllowableNoise)
	, myPixelsPerTexel(aPixelsPerTexel)
	, myTextureData(aSize / aPixelsPerTexel,
		{
			{ 0, 0, 0 },
			std::chrono::nanoseconds(0),
			0,
		})
	, myThreads(nullptr)
	, mythreadCount(aWorkerThreads)
	, myStopRequested(false)
	, myIsDone(true)
	, mySize(aSize / aPixelsPerTexel)
{
	myThreads = static_cast<WorkerThread*>(::malloc(sizeof(WorkerThread) * mythreadCount));

	for (WorkerThread& thread : fisk::tools::RangeFromStartEnd(myThreads, myThreads + mythreadCount))
		new (&thread) WorkerThread(myRayCaster, myIntersector, mySky, 10000);

	Setup();
}

Raytracer::~Raytracer()
{
	for (WorkerThread& thread : fisk::tools::RangeFromStartEnd(myThreads, myThreads + mythreadCount))
		thread.~WorkerThread();

	::free(myThreads);
	myThreads = nullptr;

	myStopRequested = true;
	myOrchistratorThread.join();
}

Raytracer::TextureType& Raytracer::GetTexture()
{
	return myTextureData;
}

void Raytracer::Imgui()
{	
	using namespace std::chrono_literals;
	
	ImGui::Begin("Tracer");

	if (ImGui::CollapsingHeader("Workers", ImGuiTreeNodeFlags_DefaultOpen))
	{
		for (WorkerThread& thread : fisk::tools::RangeFromStartEnd(myThreads, myThreads + mythreadCount))
		{
			ImGui::Text("Worker %p", &thread);
			thread.Imgui();
		}
	}

	if (ImGui::CollapsingHeader("Camera"))
	{
		ImVec2 mousePos = ImGui::GetMousePos();

		if (mousePos.x > 0 && mousePos.y > 0 && mousePos.x < mySize[0] * myPixelsPerTexel && mousePos.y < mySize[1] * myPixelsPerTexel)
		{
			fisk::tools::Ray<float, 3> ray = myRayCaster.Render({ static_cast<unsigned int>(mousePos.x / myPixelsPerTexel), static_cast<unsigned int>(mousePos.y / myPixelsPerTexel) });

			ImGui::Text("Hovered pixel");
			ImGui::DragFloat3("Direction", ray.myDirection.Raw());
			ImGui::DragFloat3("Origin", ray.myOrigin.Raw());

			ImGui::Text("");

			Camera* cam = dynamic_cast<Camera*>(&myRayCaster);

			if (cam)
			{
				ImGui::Text("Camera");
				ImGui::DragFloat3("Position", cam->GetPosition().Raw());
				ImGui::DragFloat3("FocalPoint", cam->GetFocalpoint().Raw());
				ImGui::DragFloat3("Right", cam->GetCameraRight().Raw());
				ImGui::DragFloat3("Up", cam->GetCameraUp().Raw());
			}
		}
	}

	ImGui::End();
}

float Raytracer::ExpectedChange(fisk::tools::V3f aVarianceAggregate, unsigned int aSamples)
{
	fisk::tools::V3f variance = aVarianceAggregate / aSamples;

	fisk::tools::V3f stdDeviation = { std::sqrt(variance[0]), std::sqrt(variance[1]), std::sqrt(variance[2]) };

	fisk::tools::V3f gammacorrected =
	{
		std::pow(stdDeviation[0] / 255.f, 2.2f),
		std::pow(stdDeviation[1] / 255.f, 2.2f),
		std::pow(stdDeviation[2] / 255.f, 2.2f)
	};

	fisk::tools::V3f expectedChange = gammacorrected / (aSamples + 1);

	return expectedChange.Dot({ 1, 1, 1 });
}

void Raytracer::Setup()
{
	std::random_device seed;
	std::mt19937 rng(seed());

	std::uniform_real_distribution<float> dist(0, 0.1f);

	for (unsigned int x = 0; x < mySize[0]; x++)
	{
		for (unsigned int y = 0; y < mySize[1]; y++)
		{
			myTextureData.SetTexel(
				{ x, y },
				{
					{ dist(rng), dist(rng), dist(rng)},
					std::chrono::nanoseconds(0),
					0,
				});
		}
	}

	myStopRequested = false;
	myOrchistratorThread = std::move(std::thread(std::bind(&Raytracer::Run, this)));
}

void Raytracer::Run()
{
	RegionGenerator generator(mySize);


	using namespace std::chrono_literals;

	while (!myStopRequested.load(std::memory_order::acquire))
	{
		bool hasPending = false;

		for (WorkerThread& thread : fisk::tools::RangeFromStartEnd(myThreads, myThreads + mythreadCount))
		{
			while (thread.CanRender())
			{
				thread.Render(generator.Get());

				if (!generator.Next())
				{
					myIsDone = true;
					return;
				}
			}

			WorkerThread::Result result;
			while (thread.GetResult(result))
				myTextureData.SetTexel(std::get<0>(result), std::get<1>(result));

			if (thread.GetPending() > 0)
				hasPending = true;
		}

		if (myIsDone && !hasPending)
			break;
	}
}

Raytracer::WorkerThread::WorkerThread(RayCaster& aRaycaster, IIntersector& aIntersector, Sky& aSky, size_t aSamplesPerTexel)
	: myRayCaster(aRaycaster)
	, myIntersector(aIntersector)
	, mySky(aSky)
	, mySamplesPerTexel(aSamplesPerTexel)
	, myPending(0)
	, myStopRequested(false)
	, myReadHead(myRays, SampleQueueSize)
	, myWriteHead(myRays, SampleQueueSize)
	, myRenderHead(myRays, SampleQueueSize)
{
	myThread = std::move(std::thread(std::bind(&Raytracer::WorkerThread::Run, this)));
}

Raytracer::WorkerThread::~WorkerThread()
{
	StopThread();
}

void Raytracer::WorkerThread::StopThread()
{
	if (myThread.joinable())
	{
		myStopRequested = true;
		myThread.join();
	}
}

void Raytracer::WorkerThread::Imgui()
{
	ImDrawList* drawlist = ImGui::GetWindowDrawList();

	ImVec2 cellSize = { 4, 4 };

	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	ImVec2 region_max = ImGui::GetContentRegionMaxAbs();
	
		
	float width = ImMax(4.0f, region_max.x - window->DC.CursorPos.x);

	size_t samplesPerRow = width / cellSize.x;
	
	if (samplesPerRow == 0)
		return;

	ImVec2 size = { width, SampleQueueSize / samplesPerRow * cellSize.y};

	ImVec2 topLeft = ImGui::GetCursorScreenPos();

	ImGui::Dummy(size);

	std::atomic_thread_fence(std::memory_order::consume);

	for (size_t i = 0; i < SampleQueueSize; i++)
	{
		size_t x = i % samplesPerRow;
		size_t y = i / samplesPerRow;

		ImVec2 rectTopLeft = { x * cellSize.x + topLeft.x + 1, y * cellSize.y + topLeft.y + 1 };
		ImVec2 rectBottomRight = { (x + 1) * cellSize.x + topLeft.x, (y + 1) * cellSize.y + topLeft.y };

		ImColor color;

		RayState ray = myRays[i].myState.load(std::memory_order::relaxed);

		switch (ray)
		{
		case RayState::Empty:
			color = ImColor(20, 20, 20, 255);
			break;
		case RayState::Working:
			color = ImColor(100, 100, 20, 255);
			break;
		case RayState::Done:
			color = ImColor(80, 120, 80, 255);
			break;
		}

		drawlist->AddRectFilled(rectTopLeft, rectBottomRight, color);
		//drawlist->AddRect(rectTopLeft, rectBottomRight, ImColor(100, 100, 100, 255));
	}
}

bool Raytracer::WorkerThread::CanRender()
{
	return myWriteHead->myState == RayState::Empty;
}

void Raytracer::WorkerThread::Render(fisk::tools::V2ui aUV)
{
	myWriteHead->myUV = aUV;
	myWriteHead->myResult = {};
	myWriteHead->myState = RayState::Working;

	myWriteHead++;

	myPending++;
}

bool Raytracer::WorkerThread::GetResult(Result& aOut)
{
	if (myReadHead->myState != RayState::Done)
		return false;


	aOut = 
	{
		myReadHead->myUV,
		myReadHead->myResult
	};

	myReadHead->myState = RayState::Empty;
	myReadHead++;
	myPending--;

	return true;
}

size_t Raytracer::WorkerThread::GetPending()
{
	return myPending;
}

void Raytracer::WorkerThread::Run()
{
	while (!myStopRequested.load(std::memory_order::memory_order_acquire))
	{
		if (myReadHead->myState != RayState::Working)
		{
			std::this_thread::yield();
			continue;
		}

		myReadHead->myResult = RenderTexel(myReadHead->myUV);
		myReadHead->myState = RayState::Done;
		myRenderHead++;
	}
}

Raytracer::TextureType::PackedValues Raytracer::WorkerThread::RenderTexel(fisk::tools::V2ui aUV)
{
	TextureType::PackedValues out;

	using clock = std::chrono::high_resolution_clock;
	clock::time_point start = clock::now();

	std::vector<Sample> samples;
	samples.reserve(mySamplesPerTexel);

	for (size_t i = 0; i < mySamplesPerTexel; i++)
		samples.push_back(SampleTexel(aUV));

	fisk::tools::V3f& color = std::get<ColorChannel>(out);

	for (size_t i = 0; i < mySamplesPerTexel; i++)
		color += samples[i].myColor;
	
	color /= mySamplesPerTexel;

	std::vector<unsigned int> objectsHit;
	objectsHit.resize(samples.size());

	auto corutine = ConvertVectorsAsync([](const Sample& aSample)
		{
			return aSample.myObjectId;
		}, 
		objectsHit, 
		std::chrono::hours(1), 
		samples);

	while (!corutine.done())
		corutine.resume();

	std::sort(objectsHit.begin(), objectsHit.end());

	std::vector<std::vector<unsigned int>> buckets = PartitionBy(objectsHit, [](unsigned int aLeft, unsigned int aRight)
	{
		return aLeft != aRight;
	});

	unsigned int mostHit = 0;
	size_t hits = 0;

	for (auto& bucket : buckets)
	{
		if (bucket.size() > hits)
		{
			mostHit = bucket[0];
			hits = bucket.size();
		}
	}

	std::get<ObjectIdChannel>(out) = mostHit;

	std::get<TimeChannel>(out) = clock::now() - start;

	return out;
}

Raytracer::WorkerThread::Sample Raytracer::WorkerThread::SampleTexel(fisk::tools::V2ui aUV)
{
	Sample out;

	fisk::tools::Ray<float, 3> ray = myRayCaster.Render(aUV);

	for (size_t i = 0; i < MaxBounces; i++)
	{
		std::optional<Hit> hit = myIntersector.Intersect(ray);

		if (!hit)
		{
			mySky.BlendWith(out.myColor, ray);
			break;
		}

		hit->myMaterial->InteractWith(ray, *hit, out.myColor);

		if (out.myObjectId == 0)
			out.myObjectId = hit->myObjectId;
	}

	return out;
}