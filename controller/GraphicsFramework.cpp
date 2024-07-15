
#include "GraphicsFramework.h"

#include <comdef.h>

#include "tools/Time.h"

namespace fisk
{
	GraphicsFramework::GraphicsFramework(Window& aWindow, int aBufferCount)
		: myWindow(aWindow)
		, myBufferCount(aBufferCount)
	{

		DXGI_SWAP_CHAIN_DESC desc;

		memset(&desc, 0, sizeof(desc));

		desc.BufferCount = myBufferCount;
		desc.BufferDesc.Format = BACKBUFFER_FORMAT;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.OutputWindow = myWindow.GetSystemHandle();
		desc.SampleDesc.Count = 1;
		desc.Windowed = true;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		UINT flags = 0;

#ifdef _DEBUG
		GetAsyncKeyState(VK_LSHIFT);
		if (GetAsyncKeyState(VK_LSHIFT))
		{
			flags |= D3D11_CREATE_DEVICE_DEBUG;
		}
#endif // _DEBUG

		HRESULT result = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			flags,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&desc,
			&static_cast<IDXGISwapChain*&>(mySwapChain),
			&static_cast<ID3D11Device*&>(myDevice),
			nullptr,
			&static_cast<ID3D11DeviceContext*&>(myDeviceContext));

		if (FAILED(result))
		{
			LOG_SYS_CRASH("failed to create swapchain");
			return;
		}

		CreateLibrary();
		CreateBackbufferRenderTarget();

		myCloseEventHandle = myWindow.Closed.Register(std::bind(&GraphicsFramework::OnWindowClosed, this));
		myResizeEventHandle = myWindow.ResolutionChanged.Register(std::bind(&GraphicsFramework::OnResized, this, std::placeholders::_1));
	}

	ID3D11RenderTargetView* GraphicsFramework::GetBackBufferRenderTarget()
	{
		return myBackbufferRenderTarget;
	}

	void GraphicsFramework::Present(VSyncState aVSync)
	{
		fisk::tools::ScopeDiagnostic perfLock("Present");


		assert(mySwapChain);
	
		{
			fisk::tools::ScopeDiagnostic perfLock1("Present pre events");
			BeforePresent.Fire();
		}

		{
			fisk::tools::ScopeDiagnostic perfLock("Present libcall");
			switch (aVSync)
			{
			case VSyncState::Immediate:
				mySwapChain->Present(0, 0);
				break;
			case VSyncState::OnVerticalBlank:
				mySwapChain->Present(1, 0);
				break;
			default:
				break;
			}
		}

		{
			fisk::tools::ScopeDiagnostic perfLock1("Present post events");
			AfterPresent.Fire();
		}
	}

	ID3D11Device& GraphicsFramework::Device()
	{
		return *myDevice;
	}

	ContextUtility GraphicsFramework::Context()
	{
		return ContextUtility(myDeviceContext, myContextUtilityLibrary, myWindow);
	}

	void GraphicsFramework::OnWindowClosed()
	{
		// TODO
	}

	void GraphicsFramework::OnResized(tools::V2ui aNewSize)
	{
		mySwapChain->ResizeBuffers(1, aNewSize[0], aNewSize[1], BACKBUFFER_FORMAT, 0);
		CreateDepthStencil();
	}

	void GraphicsFramework::CreateLibrary()
	{
		CreateDepthStencil();

		HRESULT result = 0;

		{
			D3D11_DEPTH_STENCIL_DESC desc;

			desc.DepthEnable = false; // TODO enable depth
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

			desc.StencilEnable = false;
			desc.StencilReadMask = 0;
			desc.StencilWriteMask = 0;

			desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
			desc.FrontFace.StencilPassOp= D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

			desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
			desc.BackFace.StencilPassOp= D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

			result = myDevice->CreateDepthStencilState(&desc, &static_cast<ID3D11DepthStencilState*&>(myContextUtilityLibrary.myNoDepth));

			if (FAILED(result))
			{
				_com_error err(result);
				LOG_SYS_CRASH("failed to create no culling depth stencil state for library");
				return;
			}
		}

		{
			D3D11_DEPTH_STENCIL_DESC desc;

			desc.DepthEnable = true; // TODO enable depth
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

			desc.StencilEnable = false;
			desc.StencilReadMask = 0;
			desc.StencilWriteMask = 0;

			desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
			desc.FrontFace.StencilPassOp= D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

			desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
			desc.BackFace.StencilPassOp= D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

			result = myDevice->CreateDepthStencilState(&desc, &static_cast<ID3D11DepthStencilState*&>(myContextUtilityLibrary.myWithDepth));

			if (FAILED(result))
			{
				_com_error err(result);
				LOG_SYS_CRASH("failed to create no culling depth stencil state for library");
				return;
			}
		}

		{
			D3D11_DEPTH_STENCIL_DESC desc;

			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

			desc.StencilEnable = false;
			desc.StencilReadMask = 0;
			desc.StencilWriteMask = 0;

			desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
			desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_NEVER;

			desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
			desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

			result = myDevice->CreateDepthStencilState(&desc, &static_cast<ID3D11DepthStencilState*&>(myContextUtilityLibrary.myBackfaceDepth));

			if (FAILED(result))
			{
				_com_error err(result);
				LOG_SYS_CRASH("failed to create front faceculling depth stencil state for library");
				return;
			}
		}

		{
			D3D11_DEPTH_STENCIL_DESC desc;

			desc.DepthEnable = true;
			desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

			desc.StencilEnable = false;
			desc.StencilReadMask = 0;
			desc.StencilWriteMask = 0;

			desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
			desc.FrontFace.StencilPassOp= D3D11_STENCIL_OP_KEEP;
			desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

			desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
			desc.BackFace.StencilPassOp= D3D11_STENCIL_OP_KEEP;
			desc.BackFace.StencilFunc = D3D11_COMPARISON_NEVER;

			result = myDevice->CreateDepthStencilState(&desc, &static_cast<ID3D11DepthStencilState*&>(myContextUtilityLibrary.myFrontfaceDepth));

			if (FAILED(result))
			{
				_com_error err(result);
				LOG_SYS_CRASH("failed to create backface culling depth stencil state for library");
				return;
			}
		}

		{
			D3D11_RASTERIZER_DESC desc;

			desc.FillMode = D3D11_FILL_SOLID;
			desc.CullMode = D3D11_CULL_NONE;
			desc.FrontCounterClockwise = true;
			desc.DepthBias = 0;
			desc.DepthBiasClamp = 0.f;
			desc.SlopeScaledDepthBias = 0.f;
			desc.DepthClipEnable = false;
			desc.ScissorEnable = false;
			desc.MultisampleEnable = true;
			desc.AntialiasedLineEnable = false;

			result = myDevice->CreateRasterizerState(&desc, &static_cast<ID3D11RasterizerState*&>(myContextUtilityLibrary.myRasterAll));

			if (FAILED(result))
			{
				_com_error err(result);
				LOG_SYS_CRASH("failed to create backface culling depth stencil state for library");
				return;
			}
		}

		{
			D3D11_RASTERIZER_DESC desc;

			desc.FillMode = D3D11_FILL_SOLID;
			desc.CullMode = D3D11_CULL_FRONT;
			desc.FrontCounterClockwise = true;
			desc.DepthBias = 0;
			desc.DepthBiasClamp = 0.f;
			desc.SlopeScaledDepthBias = 0.f;
			desc.DepthClipEnable = false;
			desc.ScissorEnable = false;
			desc.MultisampleEnable = true;
			desc.AntialiasedLineEnable = false;

			result = myDevice->CreateRasterizerState(&desc, &static_cast<ID3D11RasterizerState*&>(myContextUtilityLibrary.myRasterBackface));

			if (FAILED(result))
			{
				_com_error err(result);
				LOG_SYS_CRASH("failed to create backface culling depth stencil state for library");
				return;
			}
		}

		{
			D3D11_RASTERIZER_DESC desc;

			desc.FillMode = D3D11_FILL_SOLID;
			desc.CullMode = D3D11_CULL_BACK;
			desc.FrontCounterClockwise = true;
			desc.DepthBias = 0;
			desc.DepthBiasClamp = 0.f;
			desc.SlopeScaledDepthBias = 0.f;
			desc.DepthClipEnable = false;
			desc.ScissorEnable = false;
			desc.MultisampleEnable = true;
			desc.AntialiasedLineEnable = false;

			result = myDevice->CreateRasterizerState(&desc, &static_cast<ID3D11RasterizerState*&>(myContextUtilityLibrary.myRasterFrontface));

			if (FAILED(result))
			{
				_com_error err(result);
				LOG_SYS_CRASH("failed to create backface culling depth stencil state for library");
				return;
			}
		}

		{
			D3D11_BLEND_DESC desc;

			desc.AlphaToCoverageEnable = false;
			desc.IndependentBlendEnable = false;

			for (size_t i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
			{
				desc.RenderTarget[i].BlendEnable = false;

				desc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
				desc.RenderTarget[i].DestBlend = D3D11_BLEND_ZERO;

				desc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_MAX;

				desc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
				desc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;

				desc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_MAX;

				desc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			}

			result = myDevice->CreateBlendState(&desc, &static_cast<ID3D11BlendState*&>(myContextUtilityLibrary.myBlendOpaque));

			if (FAILED(result))
			{
				_com_error err(result);
				LOG_SYS_CRASH("failed to create opaque blend state for library");
				return;
			}
		}

		{
			D3D11_BLEND_DESC desc;

			desc.AlphaToCoverageEnable = true;
			desc.IndependentBlendEnable = false;

			for (size_t i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
			{
				desc.RenderTarget[i].BlendEnable = false;

				desc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
				desc.RenderTarget[i].DestBlend = D3D11_BLEND_ZERO;

				desc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_MAX;

				desc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
				desc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;

				desc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_MAX;

				desc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			}

			result = myDevice->CreateBlendState(&desc, &static_cast<ID3D11BlendState*&>(myContextUtilityLibrary.myBlendAlpha));

			if (FAILED(result))
			{
				_com_error err(result);
				LOG_SYS_CRASH("failed to create opaque blend state for library");
				return;
			}
		}

		{
			D3D11_SAMPLER_DESC desc;
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			
			desc.MipLODBias = 0;
			desc.MaxAnisotropy = 1;

			desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
			desc.BorderColor[0] = 0.f;
			desc.BorderColor[1] = 0.f;
			desc.BorderColor[2] = 0.f;
			desc.BorderColor[3] = 0.f;

			desc.MinLOD = 0;
			desc.MaxLOD = D3D11_FLOAT32_MAX;

			result = myDevice->CreateSamplerState(&desc, &static_cast<ID3D11SamplerState*&>(myContextUtilityLibrary.mySampleWrapPoint));

			if (FAILED(result))
			{
				LOG_SYS_CRASH("failed to create opaque blend state for library");
				return;
			}
		}

		{
			D3D11_SAMPLER_DESC desc;
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			
			desc.MipLODBias = 0;
			desc.MaxAnisotropy = 1;

			desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
			desc.BorderColor[0] = 0.f;
			desc.BorderColor[1] = 0.f;
			desc.BorderColor[2] = 0.f;
			desc.BorderColor[3] = 0.f;

			desc.MinLOD = 0;
			desc.MaxLOD = D3D11_FLOAT32_MAX;

			result = myDevice->CreateSamplerState(&desc, &static_cast<ID3D11SamplerState*&>(myContextUtilityLibrary.mySampleWrapLinear));

			if (FAILED(result))
			{
				LOG_SYS_CRASH("failed to create opaque blend state for library");
				return;
			}
		}

		{
			D3D11_SAMPLER_DESC desc;
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;

			desc.MipLODBias = 0;
			desc.MaxAnisotropy = 1;

			desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
			desc.BorderColor[0] = 0.f;
			desc.BorderColor[1] = 0.f;
			desc.BorderColor[2] = 0.f;
			desc.BorderColor[3] = 0.f;

			desc.MinLOD = 0;
			desc.MaxLOD = D3D11_FLOAT32_MAX;

			result = myDevice->CreateSamplerState(&desc, &static_cast<ID3D11SamplerState*&>(myContextUtilityLibrary.mySampleMirrorPoint));

			if (FAILED(result))
			{
				LOG_SYS_CRASH("failed to create opaque blend state for library");
				return;
			}
		}

		{
			D3D11_SAMPLER_DESC desc;
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;

			desc.MipLODBias = 0;
			desc.MaxAnisotropy = 1;

			desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
			desc.BorderColor[0] = 0.f;
			desc.BorderColor[1] = 0.f;
			desc.BorderColor[2] = 0.f;
			desc.BorderColor[3] = 0.f;

			desc.MinLOD = 0;
			desc.MaxLOD = D3D11_FLOAT32_MAX;

			result = myDevice->CreateSamplerState(&desc, &static_cast<ID3D11SamplerState*&>(myContextUtilityLibrary.mySampleMirrorLinear));

			if (FAILED(result))
			{
				LOG_SYS_CRASH("failed to create opaque blend state for library");
				return;
			}
		}

		{
			D3D11_SAMPLER_DESC desc;
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

			desc.MipLODBias = 0;
			desc.MaxAnisotropy = 1;

			desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
			desc.BorderColor[0] = 0.f;
			desc.BorderColor[1] = 0.f;
			desc.BorderColor[2] = 0.f;
			desc.BorderColor[3] = 0.f;

			desc.MinLOD = 0;
			desc.MaxLOD = D3D11_FLOAT32_MAX;

			result = myDevice->CreateSamplerState(&desc, &static_cast<ID3D11SamplerState*&>(myContextUtilityLibrary.mySampleClampPoint));

			if (FAILED(result))
			{
				LOG_SYS_CRASH("failed to create opaque blend state for library");
				return;
			}
		}

		{
			D3D11_SAMPLER_DESC desc;
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

			desc.MipLODBias = 0;
			desc.MaxAnisotropy = 1;

			desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
			desc.BorderColor[0] = 0.f;
			desc.BorderColor[1] = 0.f;
			desc.BorderColor[2] = 0.f;
			desc.BorderColor[3] = 0.f;

			desc.MinLOD = 0;
			desc.MaxLOD = D3D11_FLOAT32_MAX;

			result = myDevice->CreateSamplerState(&desc, &static_cast<ID3D11SamplerState*&>(myContextUtilityLibrary.mySampleClampLinear));

			if (FAILED(result))
			{
				LOG_SYS_CRASH("failed to create opaque blend state for library");
				return;
			}
		}
	}

	void GraphicsFramework::CreateDepthStencil()
	{
		tools::V2ui size = myWindow.GetWindowSize();

		myContextUtilityLibrary.myDepthStencil = nullptr;
		COMObject<ID3D11Texture2D> texture = nullptr;

		constexpr DXGI_FORMAT format = DXGI_FORMAT_D24_UNORM_S8_UINT;

		D3D11_TEXTURE2D_DESC textureDesc;
		textureDesc.Width = size[0];
		textureDesc.Height = size[1];
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = format;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;

		HRESULT result = myDevice->CreateTexture2D(&textureDesc, nullptr, &static_cast<ID3D11Texture2D*&>(texture));

		if (FAILED(result))
		{
			_com_error err(result);
			LOG_SYS_CRASH("Failed to create depth stencil view texture for library");
			return;
		}

		D3D11_DEPTH_STENCIL_VIEW_DESC depthDesc;
		depthDesc.Format = format;
		depthDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthDesc.Flags = 0;
		depthDesc.Texture2D.MipSlice = 0;

		result = myDevice->CreateDepthStencilView(texture, &depthDesc, &static_cast<ID3D11DepthStencilView*&>(myContextUtilityLibrary.myDepthStencil));

		if (FAILED(result))
		{
			_com_error err(result);
			LOG_SYS_CRASH("Failed to create depth stencil view for library");
			return;
		}
	}

	void GraphicsFramework::CreateBackbufferRenderTarget()
	{
		COMObject<ID3D11Texture2D> backBuffer = nullptr;
		HRESULT result = mySwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&static_cast<ID3D11Texture2D*&>(backBuffer));

		if (FAILED(result))
		{
			_com_error err(result);
			LOG_SYS_CRASH("failed to get backbuffer");
			return;
		}

		D3D11_RENDER_TARGET_VIEW_DESC desc;

		desc.Format = BACKBUFFER_FORMAT;
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipSlice = 0;

		result = myDevice->CreateRenderTargetView(backBuffer, &desc, &static_cast<ID3D11RenderTargetView*&>(myBackbufferRenderTarget));

		if (FAILED(result))
		{
			LOG_SYS_CRASH("failed to create render target for backbuffer");
			return;
		}
	}

	ContextUtility::ContextUtility(ID3D11DeviceContext* aContext, ContextUtilityLibrary& aLibrary, Window& aWindow)
		: myLibrary(aLibrary)
		, myContext(aContext)
		, myWindow(aWindow)
	{
	}

	void ContextUtility::ClearDepth(float aValue)
	{
		myContext->ClearDepthStencilView(myLibrary.myDepthStencil, D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH, aValue, 0);
	}

	void ContextUtility::ClearRenderTarget(ID3D11RenderTargetView* aTexture, tools::V4f aColor)
	{
		myContext->ClearRenderTargetView(aTexture, aColor.Raw());
	}

	void ContextUtility::SetRenderTarget(ID3D11RenderTargetView* aTexture, tools::V2ui aViewportSize, tools::V2ui aViewportOffset)
	{
		tools::V2ui size = aViewportSize;

		if (size == tools::V2ui(0, 0))
		{
			size = myWindow.GetWindowSize();
		}

		D3D11_VIEWPORT viewPort;

		viewPort.TopLeftX = aViewportOffset[0];
		viewPort.TopLeftY = aViewportOffset[1];

		viewPort.Width = size[0];
		viewPort.Height = size[1];

		viewPort.MinDepth = 0.f;
		viewPort.MaxDepth = 1.f;

		myContext->RSSetViewports(1, &viewPort);
		myContext->OMSetRenderTargets(1, &aTexture, myLibrary.myDepthStencil);
	}

	void ContextUtility::SetPixelShader(ID3D11PixelShader* aShader)
	{
		myContext->PSSetShader(aShader, nullptr, 0);
	}

	void ContextUtility::SetShaderData(ID3D11Buffer* aShaderBuffer, const std::vector<char>& aData)
	{
		HRESULT result;

		D3D11_MAPPED_SUBRESOURCE resource;
		result = myContext->Map(aShaderBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);

		if (FAILED(result))
		{
			_com_error err(result);
			LOG_SYS_CRASH("Failed to map shader buffer");
			return;
		}

		memcpy(resource.pData, aData.data(), aData.size());

		myContext->Unmap(aShaderBuffer, 0);


		myContext->PSSetConstantBuffers(0, 1, &aShaderBuffer);
		myContext->VSSetConstantBuffers(0, 1, &aShaderBuffer);
	}

	void ContextUtility::SetCulling(Culling aCulling)
	{
		switch (aCulling)
		{
		case fisk::ContextUtility::Culling::None:
			myContext->OMSetDepthStencilState(myLibrary.myNoDepth, 0);
			myContext->RSSetState(myLibrary.myRasterAll);
			break;
		case fisk::ContextUtility::Culling::DepthOnly:
			myContext->OMSetDepthStencilState(myLibrary.myWithDepth, 0);
			myContext->RSSetState(myLibrary.myRasterAll);
			break;
		case fisk::ContextUtility::Culling::Backface:
			myContext->OMSetDepthStencilState(myLibrary.myBackfaceDepth, 0);
			myContext->RSSetState(myLibrary.myRasterBackface);
			break;
		case fisk::ContextUtility::Culling::Frontface:
			myContext->OMSetDepthStencilState(myLibrary.myFrontfaceDepth, 0);
			myContext->RSSetState(myLibrary.myRasterFrontface);
			break;
		default:
			break;
		}
	}

	void ContextUtility::SetBlendmode(BlendMode aMode)
	{
		switch (aMode)
		{
		case fisk::ContextUtility::BlendMode::Opaque:
			myContext->OMSetBlendState(myLibrary.myBlendOpaque, nullptr, 0xFFFFFFFF);
			break;
		case fisk::ContextUtility::BlendMode::Aplha:
			myContext->OMSetBlendState(myLibrary.myBlendAlpha, nullptr, 0xFFFFFFFF);
			break;
		default:
			break;
		}
	}

	void ContextUtility::SetTopology(Topology aTopology)
	{
		switch (aTopology)
		{
		case fisk::ContextUtility::Topology::PointList:
			myContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
			break;
		case fisk::ContextUtility::Topology::TriangleList:
			myContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			break;
		default:
			assert(false);
			break;
		}
	}

	void ContextUtility::SetSampling(SamplerState aSampler, size_t aSlot)
	{
		switch (aSampler)
		{
		case fisk::ContextUtility::SamplerState::Wrap_Point:
			myContext->PSSetSamplers(aSlot, 1, &static_cast<ID3D11SamplerState*&>(myLibrary.mySampleWrapPoint));
			break;
		case fisk::ContextUtility::SamplerState::Wrap_Linear:
			myContext->PSSetSamplers(aSlot, 1, &static_cast<ID3D11SamplerState*&>(myLibrary.mySampleWrapLinear));
			break;
		case fisk::ContextUtility::SamplerState::Mirror_Point:
			myContext->PSSetSamplers(aSlot, 1, &static_cast<ID3D11SamplerState*&>(myLibrary.mySampleMirrorPoint));
			break;
		case fisk::ContextUtility::SamplerState::Mirror_Linear:
			myContext->PSSetSamplers(aSlot, 1, &static_cast<ID3D11SamplerState*&>(myLibrary.mySampleMirrorLinear));
			break;
		case fisk::ContextUtility::SamplerState::Clamp_Point:
			myContext->PSSetSamplers(aSlot, 1, &static_cast<ID3D11SamplerState*&>(myLibrary.mySampleClampPoint));
			break;
		case fisk::ContextUtility::SamplerState::Clamp_Linear:
			myContext->PSSetSamplers(aSlot, 1, &static_cast<ID3D11SamplerState*&>(myLibrary.mySampleClampLinear));
			break;
		}
	}

	void ContextUtility::Render(size_t aIndexCount)
	{
		myContext->DrawIndexed((UINT)aIndexCount, 0, 0);
	}

	ID3D11DeviceContext& ContextUtility::Raw()
	{
		return *myContext;
	}
}