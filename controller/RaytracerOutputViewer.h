#pragma once

#include "ConvertVector.h"

#include "GraphicsFramework.h"
#include "Raytracer.h"

#include "COMObject.h"

#include <d3d11.h>

#include <optional>

class RaytracerOutputViewer
{
public:
	RaytracerOutputViewer(fisk::GraphicsFramework& aFramework, Raytracer::TextureType& aTexture, fisk::tools::V2ui aWindowSize);

	void DrawImage();
	void Imgui();

private:

	void FlushImage(bool aRestart);
	void FlushImageData(const std::vector<fisk::tools::V3f>& aData);
	void CreateGraphicsResources();

	fisk::tools::V3f ObjectIdToColor(unsigned int aObjectId);
	fisk::tools::V3f MonoChannelColor(float aValue);

	enum class Channel : int
	{
		Color,
		TimeTaken,
		Object
	};

	fisk::GraphicsFramework& myFramework;

	fisk::COMObject<ID3D11PixelShader> myPixelShader;
	fisk::COMObject<ID3D11VertexShader> myVertexShader;
	fisk::COMObject<ID3D11Texture2D> myTexture;
	fisk::COMObject<ID3D11ShaderResourceView> myTextureView;
	fisk::COMObject<ID3D11InputLayout> myInputLayout;

	Raytracer::TextureType& myTextureData;
	fisk::tools::V2ui myWindowSize;

	std::vector<fisk::tools::V3f> myFrameBuffer;
	
	size_t myImageversion; 
	
	ConvertVectorCoroutine myTask;

	Channel myChannel;

	Raytracer::CompactNanoSecond myTimescale;
};
