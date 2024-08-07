#pragma once

#include "ConvertVector.h"

#include "GraphicsFramework.h"
#include "RendererTypes.h"

#include "COMObject.h"

#include <d3d11.h>

#include <optional>

class RaytracerOutputViewer
{
public:
	RaytracerOutputViewer(fisk::GraphicsFramework& aFramework, fisk::tools::V2ui aWindowSize, fisk::tools::V2ui aResolution);

	void DrawImage();
	void Imgui();

	void AddTexture(const TextureType& aTexture);

private:

	void FlushImage(bool aRestart);
	void FlushImageData(const std::vector<fisk::tools::V3f>& aData);
	void CreateGraphicsResources();

	fisk::tools::V3f ObjectIdToColor(unsigned int aObjectId, unsigned int aSubObjectId);
	fisk::tools::V3f MonoChannelColor(float aValue);

	enum class Channel : int
	{
		Color,
		TimeTaken,
		Object,
		Renderer
	};

	fisk::GraphicsFramework& myFramework;

	fisk::COMObject<ID3D11PixelShader> myPixelShader;
	fisk::COMObject<ID3D11VertexShader> myVertexShader;
	fisk::COMObject<ID3D11Texture2D> myTexture;
	fisk::COMObject<ID3D11ShaderResourceView> myTextureView;
	fisk::COMObject<ID3D11InputLayout> myInputLayout;

	std::vector<const TextureType*> myTextures;
	fisk::tools::V2ui myWindowSize;
	fisk::tools::V2ui myResolution;

	std::vector<fisk::tools::V3f> myFrameBuffer;
	
	int myImageSelection;
	size_t myImageversion; 
	
	ConvertVectorCoroutine myTask;

	Channel myChannel;

	CompactNanoSecond myTimescale;
};
