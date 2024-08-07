
#include "tools/SystemValues.h"

#include "RenderClient.h"
#include "NodeLimits.h"
#include "Version.h"

#include <iostream>

RenderClient::RenderClient(RenderConfig aConfig, fisk::tools::V2ui aResolution, std::string aScene, std::shared_ptr<fisk::tools::TCPSocket> aSocket)
	: myTexture(aResolution, {})
	, myState(State::SendSystemvalues)
	, mySocket(aSocket)
	, myReader(aSocket->GetReadStream())
	, myWriter(aSocket->GetWriteStream())
	, myRenderConfig(aConfig)
{
	myScene = Scene::FromFile(aScene, aResolution);
}

void RenderClient::Update()
{
	if (myState == State::Failure)
		return;
	if (myState == State::Done)
		return;

	if (!mySocket->Update())
	{
		Fail("Socket error");
		return;
	}

	switch (myState)
	{
	case RenderClient::State::SendSystemvalues:
		StepSendSystemValues();
		break;
	case RenderClient::State::ValidatingSystemvalues:
		StepValidateSystemValues();
		break;
	case RenderClient::State::SendVersion:
		StepSendVersion();
		break;
	case RenderClient::State::ValidateVersion:
		StepValidateVersion();
		break;
	case RenderClient::State::SendRenderConfig:
		StepSendRenderConfig();
		break;
	case RenderClient::State::RecvLimits:
		StepRecvLimits();
		break;
	case RenderClient::State::SendScene:
		StepSendScene();
		break;
	case RenderClient::State::StartRendering:
		StepStartRendering();
		break;
	case RenderClient::State::Running:
		StepRunning();
		break;
	case RenderClient::State::FinishUp:
		StepFinishUp();
		break;
	}
}

const TextureType& RenderClient::GetTexture()
{
	return myTexture;
}

void RenderClient::StepSendSystemValues()
{
	Log("Sending System values");
	fisk::tools::SystemValues local{};
	myWriter.DataProcessor::Process(local);

	myState = State::ValidatingSystemvalues;
}

void RenderClient::StepValidateSystemValues()
{
	fisk::tools::SystemValues remote;

	if (!myReader.ProcessAndCommit(remote))
		return;

	if (remote != fisk::tools::SystemValues{})
	{
		Fail("System values do not match");
		return;
	}

	Log("System values validated");
	myState = State::SendVersion;
}

void RenderClient::StepSendVersion()
{
	std::string version = GlobalGitVersion;
	myWriter.Process(version);

	Log("Send version");
	myState = State::ValidateVersion;
}

void RenderClient::StepValidateVersion()
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
	myState = State::SendRenderConfig;
}

void RenderClient::StepSendRenderConfig()
{
	Log("Sending render config");
	myWriter.DataProcessor::Process(myRenderConfig);
	myState = State::RecvLimits;
}

void RenderClient::StepRecvLimits()
{
	if (!myReader.ProcessAndCommit(myLimits))
		return;

	Log("Limits recieved");
	myState = State::SendScene;
}

void RenderClient::StepSendScene()
{
	Log("Sending scene");
	myWriter.DataProcessor::Process(myScene);
	myState = State::StartRendering;
}

void RenderClient::StepStartRendering()
{
	myRenderer.emplace(myLimits.myMaxPending, mySocket->GetReadStream(), mySocket->GetWriteStream());

	myOrcherstrator.emplace(myTexture, *myRenderer);

	Log("Rendering started");
	myState = State::Running;
}

void RenderClient::StepRunning()
{
	if (!myOrcherstrator->Update())
		myState = State::FinishUp;
}

void RenderClient::StepFinishUp()
{
	mySocket->Close();
	myState = State::Done;
}

void RenderClient::Log(std::string aMessage)
{
	std::cout << aMessage << "\n";
}

void RenderClient::Fail(std::string aReason)
{
	myState = State::Failure;
	std::cout << "Rendering failed: " << aReason << "\n";
}
