#include <windows.h>
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <D3DX11tex.h>
#pragma comment(lib, "D3DX11.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")
#include <thread>
#include <iostream>
#include <atomic> 
#include <string>
#include <vector>
#include <include/MinHook.h>
#include <TlHelp32.h>
#include <src/adb/adb.hpp>
#include <src/Overlay/Overlay.hpp>
#include <src/Overlay/Render.hpp>
#include <src/ui/ui.hpp>
#include <src/Globals.hpp>
#include <EspLines/Data/Data.hpp>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Features/Visuals/Visual.hpp>
#include <src/adb/adb_utils.hpp>

void initcmd() {
	AllocConsole();
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
	SetConsoleTitleA("Debug Console");
}

void closecmd() {
	FreeConsole();
	fclose(stdin);
	fclose(stdout);
	fclose(stderr);
}

HMODULE g_hModule;
FWork::Interface* g_pInterface = nullptr;
DWORD WINAPI Unload() {
	if (g_pInterface) {
		g_pInterface->ShutDown();
		delete g_pInterface;
		g_pInterface = nullptr;
	}

	if (MemoryUtils::ogPhysRead) {
		if (MH_DisableHook((LPVOID)MemoryUtils::ogPhysRead) != MH_OK) {
			std::cout << "Falha ao desativar o hook de PGMPhysRead!" << std::endl;
		}

		if (MH_RemoveHook((LPVOID)MemoryUtils::ogPhysRead) != MH_OK) {
			std::cout << "Falha ao remover o hook de PGMPhysRead!" << std::endl;
		}
	}

	MH_Uninitialize();

	if (g_hModule) {
		FreeLibraryAndExitThread(g_hModule, 0);
	}
	return 0;
}

bool memoryinit = false;
void Memoryy() {
	auto vmm = GetModuleHandleA("BstkVMM.dll");
	if (vmm == nullptr) {
		return;
	}

	auto readFunc = (MemoryUtils::PGMPhysReadFunc)GetProcAddress(vmm, "PGMPhysRead");
	if (readFunc == nullptr) {
		return;
	}

	MH_Initialize();
	if (MH_CreateHook((LPVOID)readFunc, MemoryUtils::HookedPGMPhysRead, (LPVOID*)&MemoryUtils::ogPhysRead) != MH_OK) {
		return;
	}

	if (MH_EnableHook((LPVOID)readFunc) != MH_OK) {
		return;
	}

	int timeout = 5000;
	int elapsed = 0;

	while (MemoryUtils::vmPtr == nullptr && elapsed < timeout) {
		Sleep(1);
		elapsed++;
	}

	MemoryUtils::ogCPU = (MemoryUtils::VMMGetCpuByIdFunc)GetProcAddress(vmm, "VMMGetCpuById");
	if (MemoryUtils::ogCPU == nullptr) {
		return;
	}

	MemoryUtils::ogCast = (MemoryUtils::PGMPhysGCPtr2GCPhysFunc)GetProcAddress(vmm, "PGMPhysGCPtr2GCPhys");
	if (MemoryUtils::ogCast == nullptr) {
		return;
	}

	MemoryUtils::ogWrite = (MemoryUtils::PGMPhysSimpleWriteGCPhysFunc)GetProcAddress(vmm, "PGMPhysSimpleWriteGCPhys");
	if (MemoryUtils::ogWrite == nullptr) {
		return;
	}

	MemoryUtils::Initialize(MemoryUtils::vmPtr);
	std::cout << "Virt Memory: " << MemoryUtils::pVMAddr << std::endl;

	memoryinit = true;
}

namespace Cheat {
	void Initialize() {
		FWork::Overlay::Setup(Render::FindRenderWindow());
		FWork::Overlay::Initialize();
		Memoryy();
		if (!memoryinit) {
			MessageBox(nullptr, L"Memory initialization failed", L"Error", MB_OK | MB_ICONERROR);
		}

		std::thread([&]() { FWork::ADB::InitializeADB(); }).detach();

		if (FWork::Overlay::IsInitialized())
		{
			FWork::Interface Interface(FWork::Overlay::GetOverlayWindow(), FWork::Overlay::GetTargetWindow(), FWork::Overlay::dxGetDevice(), FWork::Overlay::dxGetDeviceContext());
			Interface.UpdateStyle();
			FWork::Overlay::SetupWindowProcHook(std::bind(&FWork::Interface::WindowProc, &Interface, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

			MSG Message;
			ZeroMemory(&Message, sizeof(Message));
			while (Message.message != WM_QUIT) {

				if (PeekMessage(&Message, FWork::Overlay::GetOverlayWindow(), NULL, NULL, PM_REMOVE))
				{
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}

				ImGui::GetIO().MouseDrawCursor = Interface.GetMenuOpen();

				if (Interface.ResizeHeight != 0 || Interface.ResizeWidht != 0)
				{
					FWork::Overlay::dxCleanupRenderTarget();
					FWork::Overlay::dxGetSwapChain()->ResizeBuffers(0, Interface.ResizeWidht, Interface.ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
					Interface.ResizeHeight = Interface.ResizeWidht = 0;
					FWork::Overlay::dxCreateRenderTarget();
				}

				Interface.HandleMenuKey();
				FWork::Overlay::UpdateWindowPos();

				static bool CaptureBypassOn = false;
				if (g_Globals.General.Capture != CaptureBypassOn)
				{
					CaptureBypassOn = g_Globals.General.Capture;
					SetWindowDisplayAffinity(hWindow, CaptureBypassOn ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);
				}

				ImGui_ImplDX11_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();
				{
					FWork::Data::Work();

					Interface.RenderGui();

					if (g_Globals.Visuals.Enable) {
						ESP::Players();
					}


					if (g_Globals.Misc.ShowAimbotFov) {
						ImColor OutlineColor = ImColor(g_Globals.Misc.AimbotFovColor[0], g_Globals.Misc.AimbotFovColor[1], g_Globals.Misc.AimbotFovColor[2], g_Globals.Misc.AimbotFovColor[3]);
						ImColor Fillcolor = ImColor(
							g_Globals.AimBot.Fillcolor[0],
							g_Globals.AimBot.Fillcolor[1],
							g_Globals.AimBot.Fillcolor[2],
							g_Globals.AimBot.Fillcolor[3]
						);

						ImGui::GetBackgroundDrawList()->AddCircleFilled(
							ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2),
							g_Globals.AimBot.Fov,
							Fillcolor,
							360
						);

						ImGui::GetBackgroundDrawList()->AddCircle(
							ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2),
							g_Globals.AimBot.Fov,
							OutlineColor,
							360
						);
					}
				}
				ImGui::EndFrame();
				ImGui::Render();
				FWork::Overlay::dxRefresh();
				ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
				FWork::Overlay::dxGetSwapChain()->Present(0, 0);

				if (g_Globals.General.ShutDown) {
					Unload();
					return;
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(g_Globals.General.Delay));
			}
		}
	}
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
#ifdef _DEBUG
	initcmd();
#endif 

	Cheat::Initialize();

	while (!g_Globals.General.ShutDown) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	closecmd();
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	switch (ul_reason_for_call) {
	case DLL_PROCESS_ATTACH:
		g_hModule = hModule;
		DisableThreadLibraryCalls(hModule);
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)wWinMain, hModule, 0, NULL);
		break;
	case DLL_PROCESS_DETACH:
		g_Globals.General.ShutDown = true;
		break;
	}
	return TRUE;
}

