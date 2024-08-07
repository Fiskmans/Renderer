#pragma once


#include "tools/MathVector.h"
#include "tools/Event.h"

#define NOMINMAX
#include <WinSock2.h>
#include <Windows.h>

namespace fisk
{
	class Window
	{
	public:
		Window(tools::V2ui aSize = tools::V2ui(0,0), std::string aWindowTitle = "Window", std::string aClassName = "fisk_window");
		~Window();

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;


		[[nodiscard]]
		bool ProcessEvents();
		
		HWND GetSystemHandle();
		tools::V2ui GetWindowSize();

		tools::Event<tools::V2ui> ResolutionChanged;
		tools::Event<> Closed;

		tools::ShortCircutableEvent<LRESULT, HWND, UINT, WPARAM, LPARAM> WndProcEvent;

		// Do no call
		void SetHWND(HWND aHWND);
		LRESULT OnWndProc(UINT aUMsg, WPARAM aWParam, LPARAM aLParam);

	private:

		HWND myWindowHandle = NULL;
		tools::V2ui myWindowSize;
	};
}