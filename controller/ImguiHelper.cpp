
#include "ImguiHelper.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace fisk {

	ImguiHelper::ImguiHelper(GraphicsFramework& aFramework, Window& aWindow)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui_ImplDX11_Init(&aFramework.Device(), &aFramework.Context().Raw());
		ImGui_ImplWin32_Init(aWindow.GetSystemHandle());

		ImGui::StyleColorsDark();

		myBeforePresentEventReg = aFramework.BeforePresent.Register(std::bind(&ImguiHelper::BeforePresent, this));
		myAfterPresentEventReg = aFramework.AfterPresent.Register(std::bind(&ImguiHelper::AfterPresent, this));
		myWndProcEventReg = aWindow.WndProcEvent.Register(std::bind(&ImguiHelper::WndProc, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
	}

	ImguiHelper::~ImguiHelper()
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	void ImguiHelper::BeforePresent()
	{
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		DrawImgui.Fire();

		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}

	void ImguiHelper::AfterPresent()
	{
	}

	std::optional<LRESULT> ImguiHelper::WndProc(HWND aHWND, UINT aUMsg, WPARAM aWParam, LPARAM aLParam)
	{
		LRESULT result = ImGui_ImplWin32_WndProcHandler(aHWND, aUMsg, aWParam, aLParam);
		if (result == TRUE)
			return result;

		return {};
	}
}