

#pragma once

#include "tools/MathVector.h"

template<class TexelType>
class IRenderer
{
public:
	using Result = TexelType;

	virtual Result Render(fisk::tools::V2ui aUV) = 0;

	virtual void Update() { }
};

template<class TexelType>
class IAsyncRenderer
{
public:
	using Result = std::pair<fisk::tools::V2ui, TexelType>;

	virtual bool CanRender()
	{
		return true;
	}

	virtual void Render(fisk::tools::V2ui aUV) = 0;
	virtual bool GetResult(Result& aOut) = 0;

	virtual size_t GetPending() = 0;

	virtual void Update() { }
};