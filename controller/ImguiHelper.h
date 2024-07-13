
#pragma once

#include "Window.h"
#include "GraphicsFramework.h"

namespace fisk {
	
	class ImguiHelper
	{
	public:
		ImguiHelper(GraphicsFramework& aFramework, Window& aWindow);

		ImguiHelper(const ImguiHelper&) = delete;
		ImguiHelper(ImguiHelper&&) = delete;

		ImguiHelper& operator=(ImguiHelper&&) = delete;
		ImguiHelper& operator=(const ImguiHelper&) = delete;

		~ImguiHelper();
		
		tools::Event<> DrawImgui;

	private:
		void BeforePresent();
		void AfterPresent();
		std::optional<LRESULT> WndProc(HWND aHWND, UINT aUMsg, WPARAM aWParam, LPARAM aLParam);

		tools::EventReg myBeforePresentEventReg;
		tools::EventReg myAfterPresentEventReg;
		tools::EventReg myWndProcEventReg;
	};
}