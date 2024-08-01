#pragma once

#include "IRenderer.h"

template<class PixelType>
class CheckeredRenderer : public IAsyncRenderer<PixelType>
{
public:
	CheckeredRenderer(IAsyncRenderer<PixelType>& aA, IAsyncRenderer<PixelType>& aB, size_t aGridSize);

	bool CanRender(fisk::tools::V2ui aUV) override;

	void Render(fisk::tools::V2ui aUV) override;
	bool GetResult(CheckeredRenderer<PixelType>::Result& aOut) override;

	size_t GetPending() override;

private:
	IAsyncRenderer<PixelType>& RendererForPixel(fisk::tools::V2ui aUV);

	size_t myGridSize;

	IAsyncRenderer<PixelType>& myA;
	IAsyncRenderer<PixelType>& myB;
};

template<class PixelType>
inline CheckeredRenderer<PixelType>::CheckeredRenderer(IAsyncRenderer<PixelType>& aA, IAsyncRenderer<PixelType>& aB, size_t aGridSize)
	: myA(aA)
	, myB(aB)
	, myGridSize(aGridSize)
{
}

template<class PixelType>
inline bool CheckeredRenderer<PixelType>::CanRender(fisk::tools::V2ui aUV)
{
	return RendererForPixel(aUV).CanRender(aUV);
}

template<class PixelType>
inline void CheckeredRenderer<PixelType>::Render(fisk::tools::V2ui aUV)
{
	RendererForPixel(aUV).Render(aUV);
}

template<class PixelType>
inline bool CheckeredRenderer<PixelType>::GetResult(CheckeredRenderer<PixelType>::Result& aOut)
{
	if (myA.GetResult(aOut))
		return true;
	
	if (myB.GetResult(aOut))
		return true;

	return false;
}

template<class PixelType>
inline size_t CheckeredRenderer<PixelType>::GetPending()
{
	return myA.GetPending() + myB.GetPending();
}

template<class PixelType>
inline IAsyncRenderer<PixelType>& CheckeredRenderer<PixelType>::RendererForPixel(fisk::tools::V2ui aUV)
{
	int parity = (aUV[0] / myGridSize + aUV[1] / myGridSize) % 2;

	switch (parity)
	{
	case 0:
		return myA;
	case 1:
	default:
		return myB;
	}
}
