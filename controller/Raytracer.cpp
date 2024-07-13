#include "Raytracer.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "tools/Iterators.h"
#include "ConvertVector.h"
#include "PartitionBy.h"

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
			0,
			{0,0,0}
		})
	, myThreads(nullptr)
	, mythreadCount(aWorkerThreads)
	, myStopRequested(false)
	, myIsDone(true)
	, mySize(aSize / aPixelsPerTexel)
{
	myThreads = static_cast<WorkerThread*>(::malloc(sizeof(WorkerThread) * mythreadCount));

	for (WorkerThread& thread : fisk::tools::RangeFromStartEnd(myThreads, myThreads + mythreadCount))
		new (&thread) WorkerThread(myIntersector, mySky);

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

	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ static_cast<float>(myTextureData.GetSize()[0] * myPixelsPerTexel), static_cast<float>(myTextureData.GetSize()[1] * myPixelsPerTexel) });
	ImGui::Begin("Region jobs overlay", 0, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMouseInputs);
	
	ImDrawList* drawlist = ImGui::GetWindowDrawList();
	
	if (TimedLockGuard lock{ myOrchistratorMutex, 5ms })
	{
		for (RegionJob& job : myRegionJobs)
		{
			fisk::tools::V2ui rawTopLeft = job.GetTopLeft();
			fisk::tools::V2ui rawBottomRight = job.GetBottomRight();

			ImVec2 topLeft{static_cast<float>(rawTopLeft[0] * myPixelsPerTexel), static_cast<float>(rawTopLeft[1] * myPixelsPerTexel) };
			ImVec2 bottomRight{ static_cast<float>(rawBottomRight[0] * myPixelsPerTexel), static_cast<float>(rawBottomRight[1] * myPixelsPerTexel) };

			ImColor color = ImColor(128, 0, 0, 255);

			if (&job== &myRegionJobs[0])
				color = ImColor(0, 128, 0, 255);

			drawlist->AddRect(topLeft, bottomRight, color);
			drawlist->AddText(topLeft, color, (" " + job.GetName()).c_str());
		}
	}

	ImGui::End();
	
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

	if (ImGui::CollapsingHeader("Job regions", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (TimedLockGuard lock{myOrchistratorMutex, 5ms})
		{
			for (RegionJob& job : myRegionJobs)
			{
				ImGui::Separator();
				job.Imgui();
			}
		}
		else
		{
			ImGui::Text("Resource busy");
		}
	}

	ImGui::End();
}

void Raytracer::QueueRegion(const RegionJob& aJob)
{
	assert(aJob.GetTopLeft()[0] >= 0);
	assert(aJob.GetTopLeft()[1] >= 0);
	assert(aJob.GetBottomRight()[0] <= mySize[0]);
	assert(aJob.GetBottomRight()[1] <= mySize[0]);

	{
		std::lock_guard lock(myOrchistratorMutex);
			myRegionJobs.emplace_back(aJob);
	}

	myIsDone = false;
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
					0,
					{ 0,0,0 }
				});
		}
	}

	myStopRequested = false;
	myOrchistratorThread = std::move(std::thread(std::bind(&Raytracer::Run, this)));
}

void Raytracer::Run()
{
	QueueRegion(RegionJob(fisk::tools::V2ui{ 0, 0 }, mySize, 1, "Scratch pass"));
	QueueRegion(RegionJob(fisk::tools::V2ui{ 0, 0 }, mySize, 2, "Scratch improvement"));
	QueueRegion(RegionJob(fisk::tools::V2ui{ 0, 0 }, mySize, 1000, "Finalize"));

	using namespace std::chrono_literals;

	while (!myStopRequested.load(std::memory_order::acquire))
	{
		try
		{
			MergeOuput();
			EnqueueTexels();

			std::this_thread::yield();
		}
		catch (const DoneException&)
		{
			break;
		}
	}
}

Raytracer::RayJob Raytracer::CurrentRay()
{
	assert(!myRegionJobs.empty());

	using namespace std::chrono_literals;

	fisk::tools::V2ui at = myRegionJobs.front().Get();

	RayJob job;

	job.myColor = { 1, 1, 1 };
	job.myOriginTexel = at;

	job.myRay = myRayCaster.Render(at);

	job.myBounces = 0;
	job.myFirstObject = 0;
	job.myTimeSpent = 0ns;

	return job;
}

void Raytracer::EnqueueTexels()
{
	std::lock_guard lock(myOrchistratorMutex);
	if (myIsDone)
	{
		FindMoreWork();
		return;
	}

	for (WorkerThread& thread : fisk::tools::RangeFromStartEnd(myThreads, myThreads + mythreadCount))
	{
		for (size_t i = 0; i < WorkerThread::SampleEnqueueBatchSize; i++)
		{
			if (!thread.TryQueue(CurrentRay()))
				break;

			if (!NextSample())
			{
				myIsDone = true;
				return;
			}
		}
	}
}

bool Raytracer::NextSample()
{
	while (!myRegionJobs.empty())
	{
		if (myRegionJobs.front().Next())
			return true;

		myRegionJobs.erase(myRegionJobs.begin());
	}

	return false;
}

void Raytracer::FindMoreWork()
{
	constexpr size_t WorstAmount = 512;

	for (WorkerThread& thread : fisk::tools::RangeFromStartEnd(myThreads, myThreads + mythreadCount))
		if (thread.IsWorking())
			return;

	struct SortableTexelChange
	{
		float myExpectedChange;
		fisk::tools::V2ui myUV;

		auto operator < (const SortableTexelChange& aOther) const
		{
			return myExpectedChange > aOther.myExpectedChange;
		}
	};

	const std::vector<fisk::tools::V3f>& varianceAggregates = myTextureData.Channel<VarianceAggregateChannel>();
	const std::vector<unsigned int>& samples = myTextureData.Channel<SamplesChannel>();

	std::vector<SortableTexelChange> texels;

	texels.resize(samples.size());

	for (unsigned int x = 0; x < mySize[0]; x++)
	{
		for (unsigned int y = 0; y < mySize[1]; y++)
		{
			size_t index = x + y * mySize[0];

			SortableTexelChange& texel = texels[index];

			texel.myExpectedChange = ExpectedChange(varianceAggregates[index], samples[index]);
			texel.myUV = { x, y };
		}
	}

	std::this_thread::sleep_for(std::chrono::seconds(5));

	std::sort(texels.begin(), texels.end());

	if (texels[0].myExpectedChange < myAllowableNoise)
		throw DoneException("Were done, woo");

	assert(texels.size() > WorstAmount);

	texels.resize(WorstAmount);

	std::vector<fisk::tools::V2ui> justPositions;

	justPositions.reserve(WorstAmount);

	for (SortableTexelChange& texel : texels)
		justPositions.push_back(texel.myUV);

	size_t spp = 1;
	size_t size = 128;
	size_t precision = 1;

	while (!justPositions.empty())
	{
		QueueTexels(justPositions, spp,  size, "Precision " + std::to_string(precision));

		if (size == 0)
			break;

		spp *= 2;
		size /= 2;
		precision++;

		justPositions.resize(justPositions.size() / 2);
	}

	//throw std::exception("plix stop");
}

void Raytracer::QueueTexels(const std::vector<fisk::tools::V2ui>& aTexels, size_t aSamplesPerPixel, size_t aExpansion, std::string aName)
{
	std::vector<fisk::tools::V2ui> texels = aTexels;

	std::sort(texels.begin(), texels.end(), [](const fisk::tools::V2ui& aLeft, const fisk::tools::V2ui& aRight)
	{
		return aLeft[0] < aRight[0];
	});

	std::vector<std::vector<fisk::tools::V2ui>> xPartitions = PartitionBy<fisk::tools::V2ui>(texels, [aExpansion](const fisk::tools::V2ui& aLeft, const fisk::tools::V2ui& aRight)
	{
		return aLeft[0] + aExpansion * 2 < aRight[0];
	});

	std::vector<std::vector<std::vector<fisk::tools::V2ui>>> xyPartitions;

	for (auto& xPartition : xPartitions)
	{

		std::sort(xPartition.begin(), xPartition.end(), [](const fisk::tools::V2ui& aLeft, const fisk::tools::V2ui& aRight)
		{
			return aLeft[1] < aRight[1];
		});

		xyPartitions.push_back(PartitionBy<fisk::tools::V2ui>(xPartition, [aExpansion](const fisk::tools::V2ui& aLeft, const fisk::tools::V2ui& aRight)
		{
			return aLeft[1] + aExpansion * 2 < aRight[1];
		}));
	}

	struct BoundingBox
	{
		fisk::tools::V2ui topLeft = { std::numeric_limits<unsigned int>::max(), std::numeric_limits<unsigned int>::max() };
		fisk::tools::V2ui bottomRight = { 0, 0 };

		void Include(fisk::tools::V2ui aUV)
		{
			topLeft[0] = std::min(aUV[0], topLeft[0]);
			topLeft[1] = std::min(aUV[1], topLeft[1]);

			bottomRight[0] = std::max(aUV[0] + 1, bottomRight[0]);
			bottomRight[1] = std::max(aUV[1] + 1, bottomRight[1]);
		}

		BoundingBox Expand(unsigned int aAmount, fisk::tools::V2ui aMax) const
		{
			BoundingBox copy(*this);

			copy.topLeft -= { copy.topLeft[0] > aAmount ? aAmount : copy.topLeft[0], copy.topLeft[1] > aAmount ? aAmount : copy.topLeft[1] };
			copy.bottomRight += { aAmount, aAmount };

			copy.bottomRight[0] = std::min(aMax[0], copy.bottomRight[0]);
			copy.bottomRight[1] = std::min(aMax[1], copy.bottomRight[1]);

			return copy;
		}
	};

	std::vector<BoundingBox> boundingBoxes;

	for (auto& xyPart : xyPartitions)
	{
		for (auto& yPart : xyPart)
		{
			BoundingBox bb;

			for (fisk::tools::V2ui& texel : yPart)
				bb.Include(texel);

			boundingBoxes.push_back(bb);
		}
	}

	for (const BoundingBox& boundingBox : boundingBoxes)
	{
		{
			BoundingBox bb = boundingBox.Expand(aExpansion, mySize);
			QueueRegion(RegionJob(bb.topLeft, bb.bottomRight, aSamplesPerPixel, aName));
		}
	}
}

void Raytracer::MergeOuput()
{
	for (WorkerThread& thread : fisk::tools::RangeFromStartEnd(myThreads, myThreads + mythreadCount))
	{
		RayJob ray;
		while (thread.TryRetrieve(ray))
		{
			TextureType::PackedValues old = myTextureData.GetTexel(ray.myOriginTexel);

			fisk::tools::V3f normalizedColor = ray.myColor;

			if (std::get<SamplesChannel>(old) == 0)
			{
				myTextureData.SetTexel(
					ray.myOriginTexel,
					{
						normalizedColor,
						ray.myTimeSpent,
						ray.myFirstObject,
						1,
					{ 0, 0, 0 }
					});
			}
			else
			{
				auto& [color, timeSpent, firstObject, samples, variance] = old;

				float blendFactor = static_cast<float>(samples) / static_cast<float>(samples + 1);
				timeSpent = timeSpent * blendFactor + ray.myTimeSpent * (1.f - blendFactor);

				// Welford's online algorithm
				samples++;
				fisk::tools::V3f delta = normalizedColor - color;
				color += delta / samples;
				fisk::tools::V3f delta2 = normalizedColor - color;

				variance += delta * delta2;

				myTextureData.SetTexel(ray.myOriginTexel, old);
			}
		}

		std::atomic_thread_fence(std::memory_order::memory_order_release);
	}
}


Raytracer::WorkerThread::WorkerThread(IIntersector& aIntersector, Sky& aSky)
	: myIntersector(aIntersector)
	, mySky(aSky)
	, myStopRequested(false)
	, myStates{State::Empty}
	, myReadHead(0)
	, myWriteHead(0)
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

		switch (myStates[i].load(std::memory_order::relaxed))
		{
		case State::Empty:
			color = ImColor(20, 20, 20, 255);
			break;
		case State::Working:
			color = ImColor(100, 100, 20, 255);
			break;
		case State::Done:
			color = ImColor(80, 120, 80, 255);
			break;
		}

		drawlist->AddRectFilled(rectTopLeft, rectBottomRight, color);
		//drawlist->AddRect(rectTopLeft, rectBottomRight, ImColor(100, 100, 100, 255));
	}
}

bool Raytracer::WorkerThread::TryQueue(RayJob aRay)
{
	std::atomic_thread_fence(std::memory_order::memory_order_acquire);

	if (myStates[myWriteHead].load(std::memory_order::memory_order_relaxed) != State::Empty)
		return false;


	myRays[myWriteHead].store(aRay, std::memory_order::relaxed);
	myStates[myWriteHead].store(State::Working, std::memory_order::relaxed);

	myWriteHead = (myWriteHead + 1) % SampleQueueSize;

	std::atomic_thread_fence(std::memory_order::memory_order_release);

	return true;
}

bool Raytracer::WorkerThread::TryRetrieve(RayJob& aOutRay)
{
	std::atomic_thread_fence(std::memory_order::memory_order_acquire);

	if (myStates[myReadHead].load(std::memory_order::memory_order_relaxed) != State::Done)
		return false;


	aOutRay = myRays[myReadHead].load(std::memory_order::memory_order_acquire);
	myStates[myReadHead].store(State::Empty, std::memory_order::memory_order_relaxed);

	myReadHead = (myReadHead + 1) % SampleQueueSize;

	return true;
}

bool Raytracer::WorkerThread::IsWorking()
{
	std::atomic_thread_fence(std::memory_order::memory_order_acquire);
	for (size_t i = 0; i < SampleQueueSize; i++)
		if (myStates[i].load(std::memory_order::relaxed) == State::Working)
			return true;

	return false;
}

void Raytracer::WorkerThread::Run()
{
	while (!myStopRequested.load(std::memory_order::memory_order_acquire))
	{
		bool didWork = false;

		std::atomic_thread_fence(std::memory_order::memory_order_acquire);

		for (size_t i = 0; i < SampleQueueSize; i++)
		{
			if (myStates[i].load(std::memory_order::memory_order_relaxed) == State::Working)
			{
				didWork = true;
				
				if (StepRay(myRays[i]))
					myStates[i].store(State::Done, std::memory_order::memory_order_relaxed);
			}
		}

		std::atomic_thread_fence(std::memory_order::memory_order_release);

		if (!didWork)
			std::this_thread::yield();
	}
}

bool Raytracer::WorkerThread::CalculateNextRay(RayJob& aInOutJob)
{
	if (aInOutJob.myBounces > 10)
		return true;

	std::optional<Hit> hit = myIntersector.Intersect(aInOutJob.myRay);
	
	if (!hit)
	{
		mySky.BlendWith(aInOutJob.myColor, aInOutJob.myRay);
		return true;
	}

	hit->myMaterial->InteractWith(aInOutJob.myRay, *hit, aInOutJob.myColor);
	
	if (aInOutJob.myFirstObject == 0)
		aInOutJob.myFirstObject = hit->myObjectId;

	return false;
}

bool Raytracer::WorkerThread::StepRay(std::atomic<RayJob>& aJob)
{
	RayJob ray = aJob.load(std::memory_order::memory_order_relaxed);
	using clock = std::chrono::high_resolution_clock;
	clock::time_point start = clock::now();

	bool isFinalHit = CalculateNextRay(ray);

	ray.myTimeSpent += clock::now() - start;
	aJob.store(ray, std::memory_order::memory_order_relaxed);
	return isFinalHit;
}