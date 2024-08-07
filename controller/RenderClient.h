#pragma once

#include "tools/MathVector.h"
#include "tools/StreamReader.h"
#include "tools/StreamWriter.h"
#include "tools/TCPSocket.h"

#include "NetworkedRenderer.h"
#include "RendererTypes.h"
#include "Orchestrator.h"
#include "Scene.h"
#include "RenderConfig.h"
#include "NodeLimits.h"

#include <string>
#include <memory>

class RenderClient
{
public:
	RenderClient(RenderConfig aConfig, fisk::tools::V2ui aResolution, std::string aScene, std::shared_ptr<fisk::tools::TCPSocket> aSocket);

	void Update();

	const TextureType& GetTexture();
private:
	enum class State
	{
		SendSystemvalues,
		ValidatingSystemvalues,
		SendVersion,
		ValidateVersion,
		SendRenderConfig,
		RecvLimits,
		SendScene,
		StartRendering,
		Running,
		FinishUp,
		Done,
		Failure
	};

	void StepSendSystemValues();
	void StepValidateSystemValues();
	void StepSendVersion();
	void StepValidateVersion();
	void StepSendRenderConfig();
	void StepRecvLimits();
	void StepSendScene();
	void StepStartRendering();
	void StepRunning();
	void StepFinishUp();

	void Log(std::string aMessage);
	void Fail(std::string aReason);

	State myState;

	std::unique_ptr<Scene> myScene;
	RenderConfig myRenderConfig;

	NodeLimits myLimits;
	TextureType myTexture;
	std::optional<Orchestrator<TextureType>> myOrcherstrator;
	std::optional<NetworkedRendererMaster<TextureType::PackedValues>> myRenderer;
	std::shared_ptr<fisk::tools::TCPSocket> mySocket;
	fisk::tools::StreamReader myReader;
	fisk::tools::StreamWriter myWriter;
};