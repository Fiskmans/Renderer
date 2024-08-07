#pragma once

#include "IRenderer.h"
#include "RendererTypes.h"
#include "tools/Iterators.h"

#include <memory>
#include <vector>

template<class TexelType>
class RenderCollection : public IAsyncRenderer<TexelType>
{
public: 
	using CollectionType = std::vector<std::unique_ptr<IAsyncRenderer<TexelType>>>;

	RenderCollection(CollectionType&& aCollection);


	bool CanRender(fisk::tools::V2ui aUV) override;

	void Render(fisk::tools::V2ui aUV) override;
	bool GetResult(IAsyncRenderer<TexelType>::Result& aOut) override;

	size_t GetPending() override;

	void Update() override;

private:

	CollectionType myRenderers;
	fisk::tools::LoopingPointer<typename CollectionType::iterator> myNext; // member order important
	size_t myPending;
};

template<class TexelType>
inline RenderCollection<TexelType>::RenderCollection(CollectionType&& aCollection)
	: myRenderers(std::move(aCollection))
	, myNext(myRenderers.begin(), myRenderers.size())
	, myPending(0)
{
}

template<class TexelType>
inline bool RenderCollection<TexelType>::CanRender(fisk::tools::V2ui aUV)
{
	for (std::unique_ptr<IAsyncRenderer<TexelType>>& renderer : myRenderers)
	{
		if (renderer->CanRender(aUV))
			return true;
	}

	return false;
}

template<class TexelType>
inline void RenderCollection<TexelType>::Render(fisk::tools::V2ui aUV)
{
	fisk::tools::LoopingPointer<typename CollectionType::iterator> start = myNext;

	while(!(**myNext).CanRender(aUV))
	{
		++myNext;

		//assert(myNext == start); convestions make this assert weird
	}

	(**myNext).Render(aUV);
	myNext++;

	myPending++;
}

template<class TexelType>
inline bool RenderCollection<TexelType>::GetResult(IAsyncRenderer<TexelType>::Result& aOut)
{
	for (std::unique_ptr<IAsyncRenderer<TexelType>>& renderer : myRenderers)
	{
		if (renderer->GetResult(aOut))
		{
			myPending--;
			return true;
		}
	}
	return false;
}

template<class TexelType>
inline size_t RenderCollection<TexelType>::GetPending()
{
	return size_t();
}

template<class TexelType>
inline void RenderCollection<TexelType>::Update()
{
	for (std::unique_ptr<IAsyncRenderer<TexelType>>& renderer : myRenderers)
		renderer->Update();
}
