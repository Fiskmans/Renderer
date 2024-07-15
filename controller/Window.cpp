
#include "Window.h"

#include "tools/Logger.h"
#include "tools/Time.h"

namespace fisk
{
	static Window* globalCurrentlyCreating;
	static std::unordered_map<HWND, Window*> globalWindowMapping;

	static LRESULT CALLBACK WndProc(HWND aHWND, UINT aUMsg, WPARAM aWParam, LPARAM aLParam)
	{
		if (globalCurrentlyCreating)
		{
			if (aUMsg != WM_NCCREATE)
			{
				LOG_ERROR("Got unkown WM during window creation", std::to_string(aUMsg), std::to_string(aWParam), std::to_string(aLParam));
				return 0;
			}

			globalWindowMapping[aHWND] = globalCurrentlyCreating;
			globalCurrentlyCreating->SetHWND(aHWND);
			globalCurrentlyCreating = nullptr;
		}

		if (aUMsg == WM_NCDESTROY)
		{
			globalWindowMapping.erase(aHWND);
			return DefWindowProc(aHWND, aUMsg, aWParam, aLParam);
		}

		return globalWindowMapping.at(aHWND)->OnWndProc(aUMsg, aWParam, aLParam);
	}

	Window::Window(tools::V2ui aSize, std::string aWindowTitle, std::string aClassName)
	{
		myWindowSize = aSize;
		if (myWindowSize == tools::V2ui(0,0))
		{
			myWindowSize = tools::V2ui(
				static_cast<unsigned int>(GetSystemMetrics(SM_CXSCREEN)),
				static_cast<unsigned int>(GetSystemMetrics(SM_CYSCREEN))
			);
		}

		WNDCLASSA windowClass = {};
		windowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;
		windowClass.lpfnWndProc = WndProc;
		windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		windowClass.lpszClassName = aClassName.c_str();
		if (RegisterClassA(&windowClass) == 0)
		{
			LOG_SYS_ERROR("Failed to create window class");
			return;
		}

		DWORD windowStyle = WS_POPUP | WS_VISIBLE | WS_OVERLAPPED;

		globalCurrentlyCreating = this;
		
		CreateWindowExA(
			0,
			aClassName.c_str(),
			aWindowTitle.c_str(),
			windowStyle,
			0, 0, myWindowSize[0], myWindowSize[1],
			NULL,
			NULL,
			NULL,
			this);

		if (myWindowHandle == NULL)
		{
			LOG_SYS_ERROR("Failed to create window");
			return;
		}

		assert(!globalCurrentlyCreating);


		ShowWindow(myWindowHandle, SW_SHOW);
	}

	Window::~Window()
	{
		Closed.Fire();

		if (myWindowHandle != NULL)
		{
			CloseWindow(myWindowHandle);

			globalWindowMapping.erase(myWindowHandle);

			myWindowHandle = NULL;
		}
	}

	bool Window::ProcessEvents()
	{
		fisk::tools::ScopeDiagnostic perfLock("process events");

		MSG message;
		while (PeekMessage(&message, myWindowHandle, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);

			if (message.message == WM_QUIT || message.message == WM_DESTROY)
			{
				return false;
			}
		}

		return true;
	}

	HWND Window::GetSystemHandle()
	{
		return myWindowHandle;
	}

	tools::V2ui Window::GetWindowSize()
	{
		return myWindowSize;
	}

	void Window::SetHWND(HWND aHWND)
	{
		myWindowHandle = aHWND;
	}

	LRESULT Window::OnWndProc(UINT aUMsg, WPARAM aWParam, LPARAM aLParam)
	{
		switch (aUMsg)
		{
		case WM_SIZE:
			myWindowSize = tools::V2ui(LOWORD(aLParam), HIWORD(aLParam));
			ResolutionChanged.Fire(myWindowSize);
			return 0;
		}

		std::optional<LRESULT> result = WndProcEvent.Fire(myWindowHandle, aUMsg, aWParam, aLParam);
		if (result)
			return *result;

		return DefWindowProc(myWindowHandle, aUMsg, aWParam, aLParam);
	}
}