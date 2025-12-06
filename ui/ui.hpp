#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <d3dx11.h>

namespace FWork
{
	class Interface
	{
	public:
		Interface(HWND Window, HWND TargetWindow, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext) {
			Initialize(Window, TargetWindow, Device, DeviceContext);
		}
		~Interface() {
			ShutDown();
		}

		void Initialize(HWND Window, HWND TargetWindow, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext);
		void InitializeMenu();
		void UpdateStyle();
		void RenderGui();
		void WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		void HandleMenuKey();
		void ShutDown();
		bool GetMenuOpen() const { return bIsMenuOpen; }

	private:
		HWND hWindow;
		HWND hTargetWindow;
		ID3D11Device* IDevice;
		bool bIsMenuOpen = false;
		int Forms = 0;
	public:
		UINT ResizeWidht;
		UINT ResizeHeight;
	};
}