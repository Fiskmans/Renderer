
#pragma once

#include "tools/TCPSocket.h"
#include "tools/StreamReader.h"
#include "tools/StreamWriter.h"

#include "NodeLimits.h"
#include "RenderConfig.h"
#include "Scene.h"
#include "IIntersector.h"
#include "IRenderer.h"
#include "RendererTypes.h"

#include <memory>

class RenderServer
{
public:
	RenderServer(std::shared_ptr<fisk::tools::TCPSocket> aSocket);

	void Update();

	bool IsDone();

private:

	enum class State
	{
		SendSystemvalues,
		ValidatingSystemvalues,
		SendVersion,
		ValidatingVersion,
		RecvRenderConfig,
		AllocateResources,
		SendLimits,
		RecvScene,
		StartRendering,
		Running,
		Done,
		Failure
	};

	void StepSendSystemValues();
	void StepValidateSystemValues();
	void StepSendVersion();
	void StepValidateVersion();
	void StepRecvRenderConfig();
	void StepAllocateResources();
	void StepSendLimits();
	void StepRecvScene();
	void StepStartRendering();
	void StepRunning();

	void Log(std::string aMessage);
	void Fail(std::string aMessage);

	State myState;
	std::shared_ptr<fisk::tools::TCPSocket> mySocket;

	NodeLimits myLimits;
	RenderConfig myRenderConfig;
	fisk::tools::StreamReader myReader;
	fisk::tools::StreamWriter myWriter;

	std::unique_ptr<Scene> myScene;
	int myAllocatedThreads;

	std::unique_ptr<IIntersector> myIntersector;
	std::unique_ptr<IRenderer<TextureType::PackedValues>> myBaseRenderer;
	std::unique_ptr<IAsyncRenderer<TextureType::PackedValues>> myRenderer;
};