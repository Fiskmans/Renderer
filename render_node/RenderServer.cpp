
#include "tools/SystemValues.h"

#include "RenderServer.h"
#include "RayRenderer.h"
#include "ThreadedRenderer.h"
#include "intersectors/ClusteredIntersector.h"
#include "RenderCollection.h"
#include "Version.h"

#include <iostream>

RenderServer::RenderServer(std::shared_ptr<fisk::tools::TCPSocket> aSocket)
	: myState(State::SendSystemvalues)
	, mySocket(aSocket)
	, myReader(aSocket->GetReadStream())
	, myWriter(aSocket->GetWriteStream())
	, myAllocatedThreads(0)
{
}

void RenderServer::Update()
{
	if (myState == State::Failure)
		return;

	if (!mySocket->Update())
	{
		Fail("Socket closed");
		return;
	}

	switch (myState)
	{
	case RenderServer::State::SendSystemvalues:
		StepSendSystemValues();
		break;
	case RenderServer::State::ValidatingSystemvalues:
		StepValidateSystemValues();
		break;
	case RenderServer::State::SendVersion:
		StepSendVersion();
		break;
	case RenderServer::State::ValidatingVersion:
		StepValidateVersion();
		break;
	case RenderServer::State::RecvRenderConfig:
		StepRecvRenderConfig();
		break;
	case RenderServer::State::AllocateResources:
		StepAllocateResources();
		break;
	case RenderServer::State::SendLimits:
		StepSendLimits();
		break;
	case RenderServer::State::RecvScene:
		StepRecvScene();
		break;
	case RenderServer::State::StartRendering:
		StepStartRendering();
		break;
	case RenderServer::State::Running:
		StepRunning();
		break;
	case RenderServer::State::Done:
	case RenderServer::State::Failure:
		break;
	}
}

bool RenderServer::IsDone()
{
	switch (myState)
	{
	case RenderServer::State::Done:
	case RenderServer::State::Failure:
		return true;
	default:
		return false;
	}
}

void RenderServer::StepSendSystemValues()
{
	Log("Sending system values");

	fisk::tools::SystemValues local{};
	myWriter.DataProcessor::Process(local);

	myState = State::ValidatingSystemvalues;
}

void RenderServer::StepValidateSystemValues()
{
	fisk::tools::SystemValues local;
	fisk::tools::SystemValues remote;

	if (!myReader.ProcessAndCommit(remote))
		return;

	fisk::tools::SystemValues::Difference differences = local.Differences(remote);

	if (differences)
	{
		Fail("System values do not match");
		std::cout << differences.ToString() << "\n";

		return;
	}

	Log("System values validated");
	myState = State::SendVersion;
}

void RenderServer::StepSendVersion()
{
	std::string version = GlobalGitVersion;
	myWriter.Process(version);

	Log("Send version");
	myState = State::ValidatingVersion;
}

void RenderServer::StepValidateVersion()
{
	std::string remote;

	if (!myReader.ProcessAndCommit(remote))
		return;

	if (remote != GlobalGitVersion)
	{
		Fail("Render node is on another version node: " + remote + " local: " + GlobalGitVersion);
		return;
	}

	Log("Version validated");
	myState = State::RecvRenderConfig;
}

void RenderServer::StepRecvRenderConfig()
{
	if (!myReader.ProcessAndCommit(myRenderConfig))
		return;

	Log("Config recieved");
	myState = State::AllocateResources;
}

void RenderServer::StepAllocateResources()
{
	Log("TODO: wait for other renders to finish if they're in progress");
	
	myAllocatedThreads = std::thread::hardware_concurrency();

	if (myAllocatedThreads == 0)
	{
		myAllocatedThreads = 1;
	}
	else if (myAllocatedThreads < 2)
	{
		myAllocatedThreads = 1;
	}
	else
	{
		myAllocatedThreads -= 1;
	}

	myLimits.myMaxPending = myAllocatedThreads * 128;
	myLimits.myThreads = myAllocatedThreads;

	myState = State::SendLimits;
}

void RenderServer::StepSendLimits()
{
	myWriter.DataProcessor::Process(myLimits);

	myState = State::RecvScene;
}

void RenderServer::StepRecvScene()
{
	if (!myReader.ProcessAndCommit(myScene))
		return;

	Log("Scene recieved");
	myState = State::StartRendering;
}

void RenderServer::StepStartRendering()
{
	switch (myRenderConfig.myMode)
	{
	case RenderConfig::RaytracedClustered:
		myIntersector = std::make_unique<ClusteredIntersector>(*myScene, 8, 8);
		myBaseRenderer = std::make_unique<RayRenderer>(*myScene, *myIntersector, myRenderConfig.mySamplesPerTexel, myRenderConfig.myRenderId);
		break;
	default:
		break;
	}
	
	std::vector<std::unique_ptr<IAsyncRenderer<TextureType::PackedValues>>> renderers;

	renderers.resize(myAllocatedThreads);

	for (auto& subRenderer : renderers)
		subRenderer = std::make_unique<ThreadedRenderer<TextureType::PackedValues, 128>>(*myBaseRenderer);
		

	myRenderer = std::make_unique<RenderCollection<TextureType::PackedValues>>(std::move(renderers));

	Log("Rendering started");
	myState = State::Running;
}

void RenderServer::StepRunning()
{
	fisk::tools::V2ui uv;

	if (myReader.ProcessAndCommit(uv))
	{
		if (myRenderer->GetPending() >= myLimits.myMaxPending)
		{
			Fail("Client exceeded budget");
			return;
		}

		if (!myRenderer->CanRender(uv))
		{
			Fail("Renderer busy");
			return;
		}

		myRenderer->Render(uv);
	}

	IAsyncRenderer<TextureType::PackedValues>::Result result;
	while (myRenderer->GetResult(result))
	{
		myWriter.DataProcessor::Process(result);
	}

}

void RenderServer::Log(std::string aMessage)
{
	std::cout << aMessage << "\n";
}

void RenderServer::Fail(std::string aMessage)
{
	myState = State::Failure;
	std::cout << "Failure: " << aMessage << "\n";
}

