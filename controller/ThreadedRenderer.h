#pragma once

#include "tools/Iterators.h"

#include "IRenderer.h"

template<class TexelType, size_t QueueSize>
class ThreadedRenderer : public IAsyncRenderer<TexelType>
{
public:
	using Result = IAsyncRenderer<TexelType>::Result;

	ThreadedRenderer(IRenderer<TexelType>& aBaseRenderer);
	~ThreadedRenderer();

	ThreadedRenderer(ThreadedRenderer&) = delete;
	ThreadedRenderer& operator=(ThreadedRenderer&) = delete;
	ThreadedRenderer(ThreadedRenderer&&) = delete;
	ThreadedRenderer& operator=(ThreadedRenderer&&) = delete;

	void StopThread();

	bool CanRender() override;

	void Render(fisk::tools::V2ui aUV) override;
	bool GetResult(Result& aOut) override;

	size_t GetPending() override;

private:

	void Run();
	
	enum class RenderSlotState : uint8_t
	{
		Empty,
		Working,
		Done
	};

	struct RenderJob
	{
		fisk::tools::V2ui	myUV;
		TexelType			myResult;
		RenderSlotState		myState;
	};

	IRenderer<TexelType>&					myBaseRenderer;		// Internal thread
	std::thread								myThread;			// External thread
	size_t									myPending;			// External thread

	RenderJob								myJobs[QueueSize];	// Shared

	fisk::tools::LoopingPointer<RenderJob>	myWriteHead;		// External thread
	fisk::tools::LoopingPointer<RenderJob>	myReadHead;			// External thread

	fisk::tools::LoopingPointer<RenderJob>	myRenderHead;		// Internal thread

	std::atomic<bool>						myStopRequested;	// Shared


};

template<class TexelType, size_t QueueSize>
inline ThreadedRenderer<TexelType, QueueSize>::ThreadedRenderer(IRenderer<TexelType>& aBaseRenderer)
	: myBaseRenderer(aBaseRenderer)
	, myPending(0)
	, myStopRequested(false)
	, myJobs{}
	, myReadHead(myJobs, QueueSize)
	, myWriteHead(myJobs, QueueSize)
	, myRenderHead(myJobs, QueueSize)
{
	myThread = std::move(std::thread(std::bind(&ThreadedRenderer::Run, this)));
}

template<class TexelType, size_t QueueSize>
inline ThreadedRenderer<TexelType, QueueSize>::~ThreadedRenderer()
{
	StopThread();
}

template<class TexelType, size_t QueueSize>
inline void ThreadedRenderer<TexelType, QueueSize>::StopThread()
{
	if (myThread.joinable())
	{
		myStopRequested = true;
		myThread.join();
	}
}

template<class TexelType, size_t QueueSize>
inline bool ThreadedRenderer<TexelType, QueueSize>::CanRender()
{
	return myWriteHead->myState == RenderSlotState::Empty;
}

template<class TexelType, size_t QueueSize>
inline void ThreadedRenderer<TexelType, QueueSize>::Render(fisk::tools::V2ui aUV)
{
	myWriteHead->myUV = aUV;
	myWriteHead->myResult = {};
	myWriteHead->myState = RenderSlotState::Working;

	myWriteHead++;

	myPending++;
}

template<class TexelType, size_t QueueSize>
inline bool ThreadedRenderer<TexelType, QueueSize>::GetResult(Result& aOut)
{
	if (myReadHead->myState != RenderSlotState::Done)
		return false;

	aOut =
	{
		myReadHead->myUV,
		myReadHead->myResult
	};

	myReadHead->myState = RenderSlotState::Empty;

	myReadHead++;

	myPending--;

	return true;
}

template<class TexelType, size_t QueueSize>
inline size_t ThreadedRenderer<TexelType, QueueSize>::GetPending()
{
	return myPending;
}

template<class TexelType, size_t QueueSize>
inline void ThreadedRenderer<TexelType, QueueSize>::Run()
{
	while (!myStopRequested.load(std::memory_order::memory_order_acquire))
	{
		if (myRenderHead->myState != RenderSlotState::Working)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}

		myRenderHead->myResult = myBaseRenderer.Render(myRenderHead->myUV);
		myRenderHead->myState = RenderSlotState::Done;
		myRenderHead++;
	}
}
