

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
	using Result = std::pair<TexelType, fisk::tools::V2ui>;

	virtual void Render(fisk::tools::V2ui aUV) = 0;
	virtual bool GetResult(Result& aOut) = 0;

	virtual void Update() { }
};