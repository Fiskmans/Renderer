
#include "tools/TCPListenSocket.h"
#include "tools/TCPSocket.h"

#include "NodeLimits.h"
#include "RayRenderer.h"
#include "NetworkedRenderer.h"
#include "RendererTypes.h"
#include "IRenderer.h"
#include "Protocol.h"
#include "RenderConfig.h"
#include "intersectors/ClusteredIntersector.h"
#include "RenderServer.h"

#include "Scene.h"

#include <thread>
#include <iostream>

int main()
{
	fisk::tools::TCPListenSocket listen(12345);

	std::vector<std::unique_ptr<RenderServer>> connections;

	fisk::tools::EventReg newConnections = listen.OnNewConnection.Register([&connections](std::shared_ptr<fisk::tools::TCPSocket> aSocket)
	{
		connections.emplace_back(std::make_unique<RenderServer>(aSocket));
	});

	std::cout << "Listening on: " << listen.GetPort() << "\n";

	while (listen.Update())
	{
		for (int i = static_cast<int>(connections.size()) - 1; i >= 0; i--)
		{
			RenderServer& server = *connections[i];
			server.Update();

			if (server.IsDone())
			{
				connections.erase(connections.begin() + i);
				continue;
			}
		}

		std::this_thread::yield();
	}
}