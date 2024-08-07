#pragma once

#include "tools/StreamReader.h"
#include "tools/StreamWriter.h"

#include "IRenderer.h"

///////////////////////////////////////////////////////// Master

template<class TexelType>
class NetworkedRendererMaster : public IAsyncRenderer<TexelType>
{
public:
	NetworkedRendererMaster(size_t aMaxPending, fisk::tools::ReadStream& aReadStream, fisk::tools::WriteStream& aWriteStream);

	bool CanRender(fisk::tools::V2ui aUV) override;

	void Render(fisk::tools::V2ui aUV) override;
	bool GetResult(IAsyncRenderer<TexelType>::Result& aOut) override;

	size_t GetPending() override;

private:
	size_t myMaxPending;
	size_t myPending;
	fisk::tools::StreamReader myStreamReader;
	fisk::tools::StreamWriter myStreamWriter;
};

template<class TexelType>
inline NetworkedRendererMaster<TexelType>::NetworkedRendererMaster(size_t aMaxPending, fisk::tools::ReadStream& aReadStream, fisk::tools::WriteStream& aWriteStream)
	: myMaxPending(aMaxPending)
	, myPending(0)
	, myStreamReader(aReadStream)
	, myStreamWriter(aWriteStream)
{
}

template<class TexelType>
inline bool NetworkedRendererMaster<TexelType>::CanRender(fisk::tools::V2ui aUV)
{
	return myPending < myMaxPending;
}

template<class TexelType>
inline void NetworkedRendererMaster<TexelType>::Render(fisk::tools::V2ui aUV)
{
	myPending++;

	aUV.Process(myStreamWriter);
}

template<class TexelType>
inline bool NetworkedRendererMaster<TexelType>::GetResult(IAsyncRenderer<TexelType>::Result& aOut)
{
	if (myStreamReader.ProcessAndCommit(aOut))
	{
		myPending--;
		return true;
	}
	return false;
}

template<class TexelType>
inline size_t NetworkedRendererMaster<TexelType>::GetPending()
{
	return myPending;
}


///////////////////////////////////////////////////////// Slave

template<class TexelType>
class NetworkedRendererSlave
{
public:
	NetworkedRendererSlave(fisk::tools::ReadStream& aReadStream, fisk::tools::WriteStream& aWriteStream, IRenderer<TexelType>& aUnderlyingRenderer);

	void Update();

private:
	IRenderer<TexelType>& myUnderlyingRenderer;

	fisk::tools::StreamReader myStreamReader;
	fisk::tools::StreamWriter myStreamWriter;
};

template<class TexelType>
inline NetworkedRendererSlave<TexelType>::NetworkedRendererSlave(fisk::tools::ReadStream& aReadStream, fisk::tools::WriteStream& aWriteStream, IRenderer<TexelType>& aUnderlyingRenderer)
	: myUnderlyingRenderer(aUnderlyingRenderer)
	, myStreamReader(aReadStream)
	, myStreamWriter(aWriteStream)
{
}

template<class TexelType>
inline void NetworkedRendererSlave<TexelType>::Update()
{
	std::pair<fisk::tools::V2ui, TexelType> result;

	while (myStreamReader.ProcessAndCommit(result.first))
	{
		result.second = myUnderlyingRenderer.Render(result.first);
		myStreamWriter.Process(result);
	}
}
