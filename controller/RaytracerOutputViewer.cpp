
#include "RaytracerOutputViewer.h"


#include "imgui/imgui.h"
#include "tools/Time.h"

#include <d3dcompiler.h>
#include <random>

RaytracerOutputViewer::RaytracerOutputViewer(fisk::GraphicsFramework& aFramework, TextureType& aTexture, fisk::tools::V2ui aWindowSize)
	: myFramework(aFramework)
	, myPixelShader(nullptr)
	, myTexture(nullptr)
	, myTextureView(nullptr)
	, myVertexShader(nullptr)
	, myInputLayout(nullptr)
	, myTextureData(aTexture)
	, myWindowSize(aWindowSize)
	, myChannel(Channel::Color)
	, myTimescale(std::chrono::microseconds(10))
	, myImageversion(0)
{
	myFrameBuffer.resize(myTextureData.GetSize()[0] * myTextureData.GetSize()[1]);
	CreateGraphicsResources();
}

void RaytracerOutputViewer::DrawImage()
{
	fisk::tools::ScopeDiagnostic perfLock1("DrawImage");

	if (myImageversion < myTextureData.GetVersion() || myTask) // slightly thread unsafe, may not realize there are new version when there is
		FlushImage(false);

	ID3D11RenderTargetView* backBuffer = myFramework.GetBackBufferRenderTarget();

	fisk::ContextUtility context = myFramework.Context();

	context.Raw().IASetInputLayout(myInputLayout);
	context.Raw().IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context.Raw().VSSetShader(myVertexShader, nullptr, 0);

	context.Raw().PSSetShader(myPixelShader, nullptr, 0);

	context.SetSampling(fisk::ContextUtility::SamplerState::Clamp_Point);

	ID3D11ShaderResourceView* views[1] = { myTextureView.GetRaw() };
	context.Raw().PSSetShaderResources(0, 1, views);

	context.SetCulling(fisk::ContextUtility::Culling::None);
	context.SetBlendmode(fisk::ContextUtility::BlendMode::Opaque);

	context.SetRenderTarget(backBuffer);


	context.Raw().Draw(3, 0);
}

void RaytracerOutputViewer::Imgui()
{
	fisk::tools::ScopeDiagnostic perfLock1("Viewer Imgui");

	using namespace std::chrono_literals;

	ImGui::Begin("Viewer");

	if (myTask)
	{
		ImGui::ProgressBar(myTask.promise().myProgress, { -1.f, 0.f }, "Scanning");
	}
	else
	{
		ImGui::ProgressBar(1);
	}

	const char* ChannelNames[] = {
		"Color",
		"Time taken",
		"Object",
		"Renderer"
	};

	if (ImGui::BeginCombo("Channel", ChannelNames[static_cast<int>(myChannel)]))
	{
		for (int i = 0; i < sizeof(ChannelNames) / sizeof(*ChannelNames); i++)
		{
			if (ImGui::Selectable(ChannelNames[i], static_cast<int>(myChannel) == i))
			{
				if (static_cast<int>(myChannel) == i)
					continue;

				myChannel = static_cast<Channel>(i);

				FlushImage(true);
			}
		}

		ImGui::EndCombo();
	}

	switch (myChannel)
	{
	case RaytracerOutputViewer::Channel::Color:
		break;
	case RaytracerOutputViewer::Channel::TimeTaken:
	{
		static int nanos = myTimescale.count();
		ImGui::InputInt("Timescale", &nanos);

		if ((!ImGui::IsItemFocused() || ImGui::IsKeyDown(ImGuiKey_Enter)) && nanos != myTimescale.count())
		{
			if (nanos > 1)
				myTimescale = CompactNanoSecond(nanos);

			FlushImage(true);
		}

		ImGui::SameLine();
		ImGui::Text("(%fns)", myTimescale.count());
	}
		break;
	default:
		break;
	}

	ImGui::Separator();
	if (ImGui::CollapsingHeader("Texel info"))
	{
		ImVec2 mousePos = ImGui::GetMousePos();

		mousePos.x *= myTextureData.GetSize()[0];
		mousePos.x /= myWindowSize[0];

		mousePos.y *= myTextureData.GetSize()[1];
		mousePos.y /= myWindowSize[1];

		if (mousePos.x > 0 && mousePos.y > 0 && mousePos.x < myTextureData.GetSize()[0] - 1 && mousePos.y < myTextureData.GetSize()[1] - 1)
		{
			auto [color, rawTimeSpent, objectId, subObjectId, rendererId] = myTextureData.GetTexel({static_cast<unsigned int>(mousePos.x), static_cast<unsigned int>(mousePos.y)});

			ImGui::Text("Color ");
			ImGui::SameLine();
			ImGui::ColorButton("Color", { color[0], color[1], color[2], 1 });

			ImGui::Text("Object: %u", objectId);
			if (subObjectId != 0)
			{
				ImGui::SameLine();
				ImGui::Text(": %u", subObjectId);
			}
			ImGui::SameLine();
			fisk::tools::V3f objectColor = ObjectIdToColor(objectId, subObjectId);
			ImGui::ColorButton("ObjectID", { objectColor[0], objectColor[1], objectColor[2], 1 });

			ImGui::Text("Renderer: %u", rendererId);
			fisk::tools::V3f rendererColor = ObjectIdToColor(rendererId, 0);
			ImGui::ColorButton("RendererID", { rendererColor[0], rendererColor[1], rendererColor[2], 1 });


			std::chrono::nanoseconds integerTimeSpent = std::chrono::duration_cast<std::chrono::nanoseconds>(rawTimeSpent);
		
			bool showRestOfTimestamp = false;

			if (integerTimeSpent > 1s)
			{
				ImGui::Text("%us ", std::chrono::duration_cast<std::chrono::seconds>(integerTimeSpent).count());
				integerTimeSpent -= std::chrono::duration_cast<std::chrono::seconds>(integerTimeSpent);
				showRestOfTimestamp = true;
			}

			if (integerTimeSpent > 1ms || showRestOfTimestamp)
			{
				if (showRestOfTimestamp)
					ImGui::SameLine();

				ImGui::Text("%ums ", std::chrono::duration_cast<std::chrono::milliseconds>(integerTimeSpent).count());
				integerTimeSpent -= std::chrono::duration_cast<std::chrono::milliseconds>(integerTimeSpent);
				showRestOfTimestamp = true;
			}

			if (integerTimeSpent > 1us || showRestOfTimestamp)
			{
				if (showRestOfTimestamp)
					ImGui::SameLine();

				ImGui::Text("%uus ", std::chrono::duration_cast<std::chrono::microseconds>(integerTimeSpent).count());
				integerTimeSpent -= std::chrono::duration_cast<std::chrono::microseconds>(integerTimeSpent);
				showRestOfTimestamp = true;
			}

			if (integerTimeSpent > 1ns || showRestOfTimestamp)
			{
				if (showRestOfTimestamp)
					ImGui::SameLine();

				ImGui::Text("%uns ", integerTimeSpent.count());
				showRestOfTimestamp = true;
			}
			else
			{
				ImGui::Text("Instant or N/A");
			}
		}
	}

	ImGui::Separator();

	ImGui::End();
}

void RaytracerOutputViewer::FlushImage(bool aRestart)
{
	using namespace std::chrono_literals;

	if (aRestart && myTask)
	{
		myTask.destroy();
		myTask = {};
	}
	
	if (!myTask)
	{
		auto colorMutator = [this](const fisk::tools::V3f& aColor) -> fisk::tools::V3f
		{
			return
			{
			};
		};

		auto timeMutator = [this](const CompactNanoSecond& aTime) -> fisk::tools::V3f
		{
			float v = static_cast<float>(aTime.count()) / static_cast<float>(myTimescale.count());

			return MonoChannelColor(v);
		};

		auto idMutator = [this](unsigned int aObjectId, unsigned int aSubObjectId) -> fisk::tools::V3f
		{
			return ObjectIdToColor(aObjectId, aSubObjectId);
		};

		auto rendererMutator = [this](unsigned int aRendererID) -> fisk::tools::V3f
		{
			return ObjectIdToColor(aRendererID, 0);
		};

		switch (myChannel)
		{
		case RaytracerOutputViewer::Channel::Color:
			myFrameBuffer = myTextureData.Channel<ColorChannel>();
			FlushImageData(myFrameBuffer);
			break;
		case RaytracerOutputViewer::Channel::TimeTaken:
			myTask = ConvertVectorsAsync(timeMutator, myFrameBuffer, 10us, myTextureData.Channel<TimeChannel>());
			break;
		case RaytracerOutputViewer::Channel::Object:
			myTask = ConvertVectorsAsync(idMutator, myFrameBuffer, 10us, myTextureData.Channel<ObjectIdChannel>(), myTextureData.Channel<SubObjectIdChannel>());
			break;
		case RaytracerOutputViewer::Channel::Renderer:
			myTask = ConvertVectorsAsync(rendererMutator, myFrameBuffer, 10us, myTextureData.Channel<RendererChannel>());
			break;
		}

		myImageversion = myTextureData.GetVersion(); // slightly thread unsafe, may not realize there are new version when there is
	}

	if (!myTask)
		return;

	if (!myTask.done())
	{
		myTask.resume();
		FlushImageData(myFrameBuffer);
	}

	if (myTask.done())
	{
		myTask.destroy();
		myTask = {};
	}
}

void RaytracerOutputViewer::FlushImageData(const std::vector<fisk::tools::V3f>& aData)
{
	fisk::tools::V2ui size = myTextureData.GetSize();

	D3D11_MAPPED_SUBRESOURCE subresource;

	myFramework.Context().Raw().Map(myTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);

	assert(subresource.RowPitch == size[0] * sizeof(aData[0]));

	::memcpy(subresource.pData, aData.data(), size[0] * size[1] * sizeof(aData[0]));

	myFramework.Context().Raw().Unmap(myTexture, 0);
}

void RaytracerOutputViewer::CreateGraphicsResources()
{
	UINT flags = 0;

	// TODO enable D3DCOMPILE_SKIP_OPTIMIZATION && D3DCOMPILE_DEBUG

#ifdef _DEBUG
	flags |= D3DCOMPILE_OPTIMIZATION_LEVEL0;
#else
	flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

	fisk::COMObject<ID3DBlob> compiled = nullptr;
	fisk::COMObject<ID3DBlob> errors = nullptr;
	HRESULT result;

	{
		const char* pixelCode = R"(
Texture2D Texture		: register(t0);
SamplerState Sampler	: register(s0);
	
struct VertexToPixel
{
	float4 position : SV_Position;
	float2 uv : TEXT_COORD;
};
	
float4 pixelShader(VertexToPixel i) : SV_Target
{
	return float4(Texture.Sample(Sampler, i.uv).xyz, 1);
}
	)";

		result = D3DCompile(
			pixelCode,
			strlen(pixelCode),
			"raytracer_copy",
			nullptr,
			nullptr,
			"pixelShader",
			"ps_5_0",
			flags,
			0,
			&static_cast<ID3DBlob*&>(compiled),
			&static_cast<ID3DBlob*&>(errors));

		if (FAILED(result))
		{
			std::string errorMessage(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize());

			LOG_SYS_CRASH("failed to compile shader", errorMessage);
			return;
		}

		result = myFramework.Device().CreatePixelShader(compiled->GetBufferPointer(), compiled->GetBufferSize(), nullptr, &static_cast<ID3D11PixelShader*&>(myPixelShader));

		if (FAILED(result))
		{
			LOG_SYS_CRASH("failed to create pixel shader");
		}
	}
	{

		const char* vertexCode = R"(
struct Vertex
{
	uint id : SV_VertexID;
};
	
struct VertexToPixel
{
	float4 position : SV_Position;
	float2 uv : TEXT_COORD;
};
	
VertexToPixel vertexShader(Vertex i)
{
	VertexToPixel o;
	
	const float4 positions[3] = 
	{
		float4(-1,-1,0,1),
		float4(3,-1,0,1),
		float4(-1,3,0,1)
	};
	
	const float2 uvs[3] = 
	{
		float2(0,1),
		float2(2,1),
		float2(0,-1)
	};
	
	o.position = positions[i.id];
	o.uv = uvs[i.id];
	
	return o;
}
	)";

		result = D3DCompile(
			vertexCode,
			strlen(vertexCode),
			"raytracer_vertex",
			nullptr,
			nullptr,
			"vertexShader",
			"vs_5_0",
			flags,
			0,
			&static_cast<ID3DBlob*&>(compiled),
			&static_cast<ID3DBlob*&>(errors));

		if (FAILED(result))
		{
			std::string errorMessage(static_cast<const char*>(errors->GetBufferPointer()), errors->GetBufferSize());

			LOG_SYS_CRASH("failed to compile shader", errorMessage);
			return;
		}

		result = myFramework.Device().CreateVertexShader(compiled->GetBufferPointer(), compiled->GetBufferSize(), nullptr, &static_cast<ID3D11VertexShader*&>(myVertexShader));

		if (FAILED(result))
		{
			LOG_SYS_CRASH("failed to create vertex shader");
		}
	}

	/*
	LPCSTR SemanticName;
	UINT SemanticIndex;
	DXGI_FORMAT Format;
	UINT InputSlot;
	UINT AlignedByteOffset;
	D3D11_INPUT_CLASSIFICATION InputSlotClass;
	UINT InstanceDataStepRate;
	*/

	D3D11_INPUT_ELEMENT_DESC inputLayoutDesc[1] = {};

	inputLayoutDesc[0].SemanticName = "SV_VertexID";
	inputLayoutDesc[0].SemanticIndex = 0;
	inputLayoutDesc[0].Format = DXGI_FORMAT_R32_UINT;
	inputLayoutDesc[0].InputSlot = 0;
	inputLayoutDesc[0].AlignedByteOffset = 0;
	inputLayoutDesc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	inputLayoutDesc[0].InstanceDataStepRate = 0;

	result = myFramework.Device().CreateInputLayout(inputLayoutDesc, sizeof(inputLayoutDesc) / sizeof(*inputLayoutDesc), compiled->GetBufferPointer(), compiled->GetBufferSize(), &static_cast<ID3D11InputLayout*&>(myInputLayout));
	if (FAILED(result))
	{
		LOG_SYS_CRASH("failed to create input layout");
	}

	fisk::tools::V2ui size = myTextureData.GetSize();

	D3D11_TEXTURE2D_DESC textureDesc;
	textureDesc.Width = size[0];
	textureDesc.Height = size[1];
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Usage = D3D11_USAGE_DYNAMIC;
	textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	textureDesc.MiscFlags = 0;

	result = myFramework.Device().CreateTexture2D(&textureDesc, nullptr, &static_cast<ID3D11Texture2D*&>(myTexture));
	if (FAILED(result))
	{
		LOG_SYS_CRASH("failed to get create texture");
	}

	D3D11_SHADER_RESOURCE_VIEW_DESC textureResourceViewDesc;

	textureResourceViewDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
	textureResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureResourceViewDesc.Texture2D.MostDetailedMip = 0;
	textureResourceViewDesc.Texture2D.MipLevels = 1;

	result = myFramework.Device().CreateShaderResourceView(myTexture, &textureResourceViewDesc, &static_cast<ID3D11ShaderResourceView*&>(myTextureView));
	if (FAILED(result))
	{
		LOG_SYS_CRASH("failed to get create texture resource view");
	}
}

fisk::tools::V3f RaytracerOutputViewer::ObjectIdToColor(unsigned int aObjectId, unsigned int aSubObjectId)
{
	using Key = size_t;
	assert(sizeof(size_t) >= sizeof(aObjectId) + sizeof(aSubObjectId));

	static std::unordered_map<Key, fisk::tools::V3f> cache;

	Key key = ((static_cast<Key>(aObjectId)) << (sizeof(aObjectId) * CHAR_BIT)) | static_cast<Key>(aSubObjectId);

	if (aObjectId == 0)
		return { 0, 0, 0 };

	decltype(cache)::const_iterator cachedIt = cache.find(key);

	if (cachedIt != cache.end())
		return cachedIt->second;

	std::mt19937 rng(aObjectId);
	std::uniform_real_distribution<float> dist(0, 0.6f);

	fisk::tools::V3f color{
		dist(rng) + 0.3f,
		dist(rng) + 0.3f,
		dist(rng) + 0.3f
	};

	if (aSubObjectId != 0)
	{
		std::mt19937 subRNG(aSubObjectId);
		std::uniform_real_distribution<float> subDist(-0.02f, 0.02f);

		color += {
			subDist(subRNG),
			subDist(subRNG),
			subDist(subRNG)
		};
	}

	cache.insert({ key, color });

	return color;
}

fisk::tools::V3f RaytracerOutputViewer::MonoChannelColor(float aValue)
{
	return { aValue / (aValue + 1), aValue / (aValue + 2), aValue / (aValue + 4) };
}
