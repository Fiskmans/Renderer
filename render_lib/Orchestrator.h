#pragma once

#include "tools/MathVector.h"
#include "tools/Trace.h"

#include "MultichannelTexture.h"

#include "IRenderer.h"
#include "RegionGenerator.h"

#include <vector>

template<class TextureType>
class Orchestrator
{
public:
	using TexelType = TextureType::PackedValues;

	Orchestrator(TextureType& aTexture, IAsyncRenderer<TexelType>& aRenderer);

	bool Update();

private:
	TextureType& myTexture;

	RegionGenerator myGenerator;
	IAsyncRenderer<TexelType>& myRenderer;
};

template<class TextureType>
inline Orchestrator<TextureType>::Orchestrator(TextureType& aTexture, IAsyncRenderer<TexelType>& aRenderer)
	: myTexture(aTexture)
	, myGenerator(aTexture.GetSize())
	, myRenderer(aRenderer)
{
}

template<class TextureType>
inline bool Orchestrator<TextureType>::Update()
{
	FISK_TRACE("update");
	if (!myGenerator.Done())
	{
		FISK_TRACE("scheduling");

		while (myRenderer.CanRender(myGenerator.Get()))
		{
			myRenderer.Render(myGenerator.Get());
				
			if (!myGenerator.Next())
				break;
		}
	}

	{
		FISK_TRACE("merging");

		typename IAsyncRenderer<TexelType>::Result result;
		while (myRenderer.GetResult(result))
		{
			myTexture.SetTexel(result.first, result.second);
		}
	}

	if (!myGenerator.Done())
		return true;

	if (myRenderer.GetPending() > 0)
		return true;

	return false;
}