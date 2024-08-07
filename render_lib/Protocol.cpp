#include "Protocol.h"

#include "tools/SystemValues.h"
#include "tools/StreamReader.h"
#include "tools/StreamWriter.h"

#include <iostream>

namespace fisk::tools
{
	Protocol::Protocol(const ProtocolSequence& aSequence, std::shared_ptr<fisk::tools::TCPSocket>& aSocket, std::unique_ptr<IProtocolPayload>&& aPayload)
	{
		myAt = std::begin(aSequence);
		myEnd = std::end(aSequence);

		mySocket = aSocket;
		myPayload = std::move(aPayload);
		myIsAborted = false;

		myDataEventHandle = aSocket->OnDataAvailable.Register(std::bind(&Protocol::Step, this));

		Step();
	}

	bool Protocol::Update()
	{
		if (!mySocket->Update())
			return false;

		if (myIsAborted)
			return false;

		if (myAt == myEnd)
			return false;

		return true;
	}

	void Protocol::Step()
	{
		if (myAt == myEnd)
			return;

		while (true)
		{
			switch ((*myAt)(*myPayload, *mySocket))
			{
			case ProtocolStepResult::Stay:
				return;

			case ProtocolStepResult::Again:
				break;

			case ProtocolStepResult::Next:
				myAt++;
				if (myAt == myEnd)
					return;

				break;

			case ProtocolStepResult::Abort:
				return;
			}
		}
	}

	namespace basic_protocols
	{
		ProtocolStepResult SendSystemValues::operator()(IProtocolPayload& aPayload, fisk::tools::TCPSocket& aSocket) const
		{
			SystemValues localValues;

			StreamWriter writer(aSocket.GetWriteStream());
			localValues.Process(writer);

			return ProtocolStepResult::Next;
		}

		ProtocolStepResult ValidateSystemValues::operator()(IProtocolPayload& aPayload, fisk::tools::TCPSocket& aSocket) const
		{
			SystemValues remoteValues;

			StreamReader reader(aSocket.GetReadStream());
			if (!reader.ProcessAndCommit(remoteValues))
				return ProtocolStepResult::Stay;

			if (remoteValues != SystemValues())
				return ProtocolStepResult::Abort;

			return ProtocolStepResult::Next;
		}
	}
}
