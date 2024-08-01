#pragma once

#include "tools/MathVector.h"
#include "tools/Time.h"

#include "MultichannelTexture.h"

#include "IRenderer.h"
#include "RegionGenerator.h"

#include <vector>

template<class TextureType>
class Orchestrator
{
public:
	using TexelType = TextureType::PackedValues;

	Orchestrator(TextureType& aTexture, std::vector<IAsyncRenderer<TexelType>*> aRenderers);

	bool Update();
	const TextureType& GetTexture();

private:
	TextureType& myTexture;

	RegionGenerator myGenerator;
	std::vector<IAsyncRenderer<TexelType>*> myRenderers;
};

template<class TextureType>
inline Orchestrator<TextureType>::Orchestrator(TextureType& aTexture, std::vector<IAsyncRenderer<TexelType>*> aRenderers)
	: myTexture(aTexture)
	, myGenerator(aTexture.GetSize())
	, myRenderers(aRenderers)
{
}

template<class TextureType>
inline bool Orchestrator<TextureType>::Update()
{
	fisk::tools::ScopeDiagnostic perfLock("update");
	if (!myGenerator.Done())
	{
		fisk::tools::ScopeDiagnostic perfLock("scheduling");

		for (auto* renderer : myRenderers)
		{
			if (myGenerator.Done())
				break;

			while (renderer->CanRender(myGenerator.Get()))
			{
				renderer->Render(myGenerator.Get());
				
				if (!myGenerator.Next())
					break;
			}
		}
	}

	bool hasPending = false;


	for (auto& renderer : myRenderers)
	{
		fisk::tools::ScopeDiagnostic perfLock("merging");

		typename IAsyncRenderer<TexelType>::Result result;
		while (renderer->GetResult(result))
		{
			myTexture.SetTexel(std::get<0>(result), std::get<1>(result));
		}

		if (renderer->GetPending() > 0)
			hasPending = true;
	}


	if (!myGenerator.Done())
		return true;

	if (hasPending)
		return true;

	return false;
}

template<class TextureType>
inline const TextureType& Orchestrator<TextureType>::GetTexture()
{
	return myTexture;
}