
#pragma once 

#include "tools/SystemValues.h"
#include "tools/StreamReader.h"
#include "tools/StreamWriter.h"
#include "tools/TCPSocket.h"

#include <unordered_map>
#include <coroutine>

namespace fisk::tools
{
	class IProtocolPayload
	{
	public:
		virtual ~IProtocolPayload() = default;

		template<std::derived_from<IProtocolPayload> T>
		T& As()
		{
			return *reinterpret_cast<T*>(this);
		}
	};

	enum class ProtocolStepResult
	{
		Stay,
		Again,
		Next,
		Abort
	};

	using ProtocolStep = std::function<ProtocolStepResult(IProtocolPayload& aPayload, fisk::tools::TCPSocket& aSocket)>;

	using ProtocolSequence = std::vector<ProtocolStep>;

	class Protocol
	{
	public:
		Protocol(const ProtocolSequence& aSequence, std::shared_ptr<fisk::tools::TCPSocket>& aSocket, std::unique_ptr<IProtocolPayload>&& aPayload);

		bool Update();

	private:
		void Step();

		std::unique_ptr<IProtocolPayload> myPayload;
		std::shared_ptr<fisk::tools::TCPSocket> mySocket;

		fisk::tools::EventReg myDataEventHandle;

		bool myIsAborted;
		ProtocolSequence::const_iterator myAt;
		ProtocolSequence::const_iterator myEnd;
	};

	namespace basic_protocols
	{
		class SendSystemValues
		{
		public:
			ProtocolStepResult operator()(IProtocolPayload& aPayload, fisk::tools::TCPSocket& aSocket) const;
		};

		class ValidateSystemValues 
		{
		public:
			ProtocolStepResult operator()(IProtocolPayload& aPayload, fisk::tools::TCPSocket& aSocket) const;
		};

	}

}