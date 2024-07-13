
#pragma once

#include "tools/Event.h"

#include "Window.h"
#include "COMObject.h"

#include <d3d11.h>

namespace fisk
{
	struct ContextUtilityLibrary
	{
		COMObject<ID3D11DepthStencilView> myDepthStencil = nullptr;

		COMObject<ID3D11DepthStencilState> myNoDepth = nullptr;
		COMObject<ID3D11DepthStencilState> myWithDepth = nullptr;
		COMObject<ID3D11DepthStencilState> myBackfaceDepth = nullptr;
		COMObject<ID3D11DepthStencilState> myFrontfaceDepth = nullptr;
		
		COMObject<ID3D11RasterizerState> myRasterAll = nullptr;
		COMObject<ID3D11RasterizerState> myRasterFrontface = nullptr;
		COMObject<ID3D11RasterizerState> myRasterBackface = nullptr;

		COMObject<ID3D11BlendState> myBlendOpaque = nullptr;
		COMObject<ID3D11BlendState> myBlendAlpha = nullptr;

		COMObject<ID3D11SamplerState> mySampleWrapPoint = nullptr;
		COMObject<ID3D11SamplerState> mySampleWrapLinear = nullptr;
		COMObject<ID3D11SamplerState> mySampleMirrorPoint = nullptr;
		COMObject<ID3D11SamplerState> mySampleMirrorLinear = nullptr;
		COMObject<ID3D11SamplerState> mySampleClampPoint = nullptr;
		COMObject<ID3D11SamplerState> mySampleClampLinear = nullptr;
	};

	class ContextUtility
	{
	public:
		ContextUtility(ID3D11DeviceContext* aContext, ContextUtilityLibrary& aLibrary, Window& aWindow);

		void ClearDepth(float aValue);

		void ClearRenderTarget(ID3D11RenderTargetView* aTexture, tools::V4f aColor);

		void SetRenderTarget(ID3D11RenderTargetView* aTexture, tools::V2ui aViewportSize = tools::V2ui(0,0), tools::V2ui aViewportOffset = tools::V2ui(0,0));

		void SetPixelShader(ID3D11PixelShader* aShader);

		void SetShaderData(ID3D11Buffer* aShaderBuffer, const std::vector<char>& aData);


		enum class Culling
		{
			None,
			DepthOnly,
			Backface,
			Frontface
		};

		void SetCulling(Culling aCulling);

		enum class BlendMode
		{
			Opaque,
			Aplha
		};

		void SetBlendmode(BlendMode aMode);

		enum class Topology
		{
			TriangleList,
			PointList
		};

		void SetTopology(Topology aTopology);

		enum class SamplerState
		{
			Wrap_Point,
			Wrap_Linear,
			Mirror_Point,
			Mirror_Linear,
			Clamp_Point,
			Clamp_Linear
		};

		void SetSampling(SamplerState aSampler, size_t aSlot = 0);

		void Render(size_t aIndexCount);

		ID3D11DeviceContext& Raw();

	private:
		ContextUtilityLibrary& myLibrary;
		ID3D11DeviceContext* myContext;
		Window& myWindow;
	};

	class GraphicsFramework
	{
	public:
		GraphicsFramework(Window& aWindow, int aBufferCount = 2);
		~GraphicsFramework() = default;

		GraphicsFramework(const GraphicsFramework&) = delete;
		GraphicsFramework& operator=(const GraphicsFramework&) = delete;

		enum class VSyncState
		{
			Immediate,
			OnVerticalBlank
		};

		tools::Event<> BeforePresent;
		tools::Event<> AfterPresent;

		ID3D11RenderTargetView* GetBackBufferRenderTarget();
		void Present(VSyncState aVSync);

		ID3D11Device& Device();
		ContextUtility Context();

	private:
		static constexpr DXGI_FORMAT BACKBUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

		void CreateLibrary();
		void CreateDepthStencil();
		void CreateBackbufferRenderTarget();

		void OnWindowClosed();
		void OnResized(tools::V2ui aNewSize);

		Window& myWindow;

		tools::EventReg myCloseEventHandle;
		tools::EventReg myResizeEventHandle;

		int myBufferCount;
		int myCurrentBuffer;

		ContextUtilityLibrary myContextUtilityLibrary;
		COMObject<ID3D11Device> myDevice = nullptr;
		COMObject<ID3D11DeviceContext> myDeviceContext = nullptr;
		COMObject<IDXGISwapChain> mySwapChain = nullptr;
		COMObject<ID3D11RenderTargetView> myBackbufferRenderTarget = nullptr;
	};
}