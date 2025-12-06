#include "ui.hpp"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <src/Overlay/Overlay.hpp>
#include <src/Globals.hpp>
#include <src/Fonts/FontInter.hpp>
#include <imgui_internal.h>
#include <src/Fonts/Fonts.hpp>
#include <thread>
#include <Auth/auth.hpp>
#define CURL_STATICLIB
#include "Auth/Curl/curl.h"
#include "Imspinner\Imspinner.h"
#include "src\Fonts\Fonts.hpp"
#include "Particles\Particles.hpp"
#include <FontAwesome6.hpp>
#include "Logo.hpp"
#include <lmcons.h>
#include <font_awesome.h>
#include <font_awesome.cpp>
#include "bytes.hpp"
#include <EspLines/Offsets.hpp>
#include <src/adb/adb_utils.hpp>
#include <include/MinHook.h>
#include <EspLines/Aimbot/Leswrite.hpp>
#include <EspLines/Aimbot/SilentVerySkidder.hpp>
static inline ImVec2 Size = ImVec2(640, 440);

int CurrentTab = 0;

char Username[255] = "";
char Password[255] = "";
char Key[255] = "";
char Gmail[255] = "";

int OldCurrentTab = 0;
ImFont* DefaultESPFont = nullptr;
ImFont* Verdana = nullptr;
ImFont* SmallFonts = nullptr;
ImFont* EspFont = nullptr;
ImFont* FontAwesomeSolidBig = nullptr;
ImFont* InterRegular14 = nullptr;
ImFont* InterBlack = nullptr;
ImFont* InterBold = nullptr;
ImFont* InterBold12 = nullptr;
ImFont* InterExtraBold = nullptr;
ImFont* InterExtraLight = nullptr;
ImFont* InterLight = nullptr;
ImFont* InterMedium = nullptr;
ImFont* InterRegular = nullptr;
ImFont* InterSemiBold = nullptr;
ImFont* InterThin = nullptr;

ImFont* FontAwesomeRegular = nullptr;
ImFont* FontAwesomeSolid = nullptr;
ImFont* FontAwesomeSolid14 = nullptr;
ImFont* FontAwesomeBrands = nullptr;

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;

HWND hwnd = nullptr;

ID3D11ShaderResourceView* Logo = nullptr;

std::string GetUsername()
{
    char username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (GetUserNameA(username, &username_len))
        return std::string(username);
    return "Unknown";
}




void AimbotTab()
{

    ImGui::SetCursorPos(ImVec2(20, 20));
    ImGui::CustomChild(("aimbot"), ImVec2(ImGui::GetWindowSize().x / 2 - 30, 200));
    {
        ImGui::Checkbox(("enable Aimbot"), &g_Globals.AimBot.leswrite);
        ImGui::SliderInt("max distance", &g_Globals.AimBot.distance, 10, 200);
        ImGui::KeyBind(("keybind"), &g_Globals.AimBot.AimbotBind);
        ImGui::Checkbox("show fov", &g_Globals.Misc.ShowAimbotFov);
    }
    ImGui::EndCustomChild();

    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 + 10, 20));
    ImGui::CustomChild(("silent aim"), ImVec2(ImGui::GetWindowSize().x / 2 - 30, 200));
    {
        ImGui::Checkbox(("enable (risk)"), &g_Globals.Silent.Enabled);
        ImGui::SliderInt("max distance", &g_Globals.Silent.MaxDistance, 10, 500);
        ImGui::Checkbox("show fov", &g_Globals.Misc.ShowAimbotFov);
        ImGui::SliderInt(("fov"), &g_Globals.AimBot.Fov, 1, 300);
    }
    ImGui::EndCustomChild();

    ImGui::SetCursorPos(ImVec2(20, 20 + 200 + 20));
    ImGui::CustomChild(("settings"), ImVec2(ImGui::GetWindowSize().x / 2 - 30, 120));
    {
        ImGui::KeyBind(("keybind"), &g_Globals.AimBot.SilentBind);
        ImGui::Checkbox(("priority distance"), &g_Globals.AimBot.PriorityDistance);
        ImGui::Checkbox(("priority health"), &g_Globals.AimBot.PriorityHealth);
        ImGui::Checkbox(("priority fov"), &g_Globals.AimBot.PriorityFov);
    }
    ImGui::EndCustomChild();
}

void VisualsTab()
{
    ImGui::SetCursorPos(ImVec2(20, 20));
    ImGui::CustomChild(("player ESP"), ImVec2(ImGui::GetWindowSize().x / 2 - 30, 370));
    {
        ImGui::Checkbox("enable esp", &g_Globals.Visuals.Enable);
        ImGui::Checkbox("enable enemies", &g_Globals.Visuals.Enemy);
        ImGui::Checkbox("esp weapons text", &g_Globals.Visuals.ESPWeapon);
        ImGui::Checkbox("esp Weapons icon", &g_Globals.Visuals.ESPWeaponIcon);
        ImGui::Checkbox("esp lines", &g_Globals.Visuals.Lines);
        ImGui::Checkbox("enable health bar", &g_Globals.Visuals.HealthBar);
        ImGui::Checkbox("health text", &g_Globals.Visuals.ESPHealthTEXT);
        ImGui::Checkbox("enable box", &g_Globals.Visuals.Box);
        ImGui::Checkbox("enable filledBox", &g_Globals.Visuals.FilledBox);
        ImGui::Checkbox("enable name", &g_Globals.Visuals.Name);
        ImGui::Checkbox("enable distance", &g_Globals.Visuals.Distance);
        ImGui::Checkbox("enable skeleton", &g_Globals.Visuals.Skeleton);
    }
    ImGui::EndCustomChild();

    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 + 10, 20));
    ImGui::CustomChild("Global Color", ImVec2(ImGui::GetWindowSize().x / 2 - 30, 210));
    {
        ImGui::ColorEdit4("name color", g_Globals.Visuals.NameColor, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("distance color", g_Globals.Visuals.DistColor, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("skeleton color", g_Globals.Visuals.SkeletonColor, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("box color", g_Globals.Visuals.BoxColor, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("lines color", g_Globals.Visuals.LinesColor, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("health color", g_Globals.Visuals.texthColor, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("enemies detected color", g_Globals.Visuals.EnemyColor, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("watermark color", g_Globals.Visuals.WatermarkColor, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("enemies knocked color", g_Globals.Visuals.WatermarkColor, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("weapons text color", g_Globals.Visuals.ESPWeaponColor, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("filled box color", g_Globals.Visuals.Filledboxcolor, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
        ImGui::ColorEdit4("target color", g_Globals.Visuals.AlvoColor, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs);
    }
    ImGui::EndCustomChild();

    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 + 10, 20 + 210 + 20));
    ImGui::CustomChild("config ESP", ImVec2(ImGui::GetWindowSize().x / 2 - 30, 140));
    {
        ImGui::Combo("lines position", &g_Globals.Visuals.EspLines, "none\0top\0bottom\0");
        ImGui::Combo("health position", &g_Globals.Visuals.players_healthbar, "none\0left\0right\0top\0bottom");
        ImGui::Combo("box style", &g_Globals.Visuals.players_box, "none\0full\0cornered\0");
        ImGui::SliderInt("render distance", &g_Globals.Visuals.DistanceEsp, 0, 250, "%dms");
    }
    ImGui::EndCustomChild();
}

void SettingsTab()
{
    ImGui::SetCursorPos(ImVec2(20, 20));
    ImGui::CustomChild(("config system"), ImVec2(ImGui::GetWindowSize().x / 2 - 30, 370));
    {
        ImGui::Checkbox("stream mode", &g_Globals.General.Capture);
       ImGui::SliderInt("delay", &g_Globals.General.Delay, 0, 100, "%dms");
       ImGui::KeyBind("keybind menu", &g_Globals.General.MenuKey);
    }
    ImGui::EndCustomChild();

    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 + 10, 20));
    ImGui::CustomChild(("extra"), ImVec2(ImGui::GetWindowSize().x / 2 - 30, 150));
    {
        if (ImGui::Button("unload cheat"))
        {
            g_Globals.General.ShutDown = true;
        }
    }
    ImGui::EndCustomChild();
}

void PlayersTab()
{
    ImGui::SetCursorPos(ImVec2(20, 20));
    ImGui::CustomChild(("exploits"), ImVec2(ImGui::GetWindowSize().x / 2 - 30, 370));
    {
        ImGui::Checkbox("up player", &g_Globals.Exploits.UpPlayer);
        ImGui::KeyBind("keyupplayer", &g_Globals.Exploits.UpPlayerBind);
        ImGui::Checkbox("down player", &g_Globals.Exploits.DownPlayer);
        ImGui::KeyBind("keydownplayer", &g_Globals.Exploits.DownPlayerBind);
        ImGui::KeyBind("KeyGhost", &g_Globals.Exploits.GhostBind);
        ImGui::Checkbox("tp wall", &g_Globals.Exploits.TpWall);
        ImGui::KeyBind("keywall", &g_Globals.Exploits.TpWallBind);
        ImGui::Checkbox("tp to me ( 8M )", &g_Globals.Exploits.TeleKill);
        ImGui::KeyBind("keytelekill", &g_Globals.Exploits.TeleKillBind);
    }
    ImGui::EndCustomChild();

    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 + 10, 20));
    ImGui::CustomChild(("extra"), ImVec2(ImGui::GetWindowSize().x / 2 - 30, 150));
    {
        ImGui::Checkbox("speed ( +1.5 m/s )", &g_Globals.Exploits.SpeedHack);
        ImGui::Checkbox("wall Hack", &g_Globals.Exploits.WallHack);
        ImGui::KeyBind("keyWallHack", &g_Globals.Exploits.WallHackBind);
    }
    ImGui::EndCustomChild();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
namespace FWork {
    void Interface::Initialize(HWND Window, HWND TargetWindow, ID3D11Device* Device, ID3D11DeviceContext* DeviceContext) {
        hWindow = Window;
        IDevice = Device;
        g_pd3dDevice = Device;
        g_pd3dDeviceContext = DeviceContext;

        ImGui::CreateContext();
        ImGui_ImplWin32_Init(hWindow);
        ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
        ImGuiStyle* Style = &ImGui::GetStyle();
        ImGuiIO io = ImGui::GetIO();
        DefaultESPFont = io.Fonts->AddFontFromMemoryTTF(neverloseFont, sizeof(neverloseFont), 12.f, 0, io.Fonts->GetGlyphRangesCyrillic());
        SmallFonts = io.Fonts->AddFontFromMemoryTTF(pixelmix_ttf, sizeof(pixelmix_ttf), 10.f, 0, io.Fonts->GetGlyphRangesCyrillic());

        static const ImWchar icons_ranges[] = { 0xf000, 0xf3ff, 0 }; 
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.OversampleH = 2.5;
        icons_config.OversampleV = 2.5;

        InterBlack = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(InterBlack_compressed_data, InterBlack_compressed_size, 14, 0, io.Fonts->GetGlyphRangesCyrillic());
        InterBold = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(InterBold_compressed_data, InterBold_compressed_size, 16, 0, io.Fonts->GetGlyphRangesCyrillic());
        InterExtraBold = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(InterExtraBold_compressed_data, InterExtraBold_compressed_size, 14, 0, io.Fonts->GetGlyphRangesCyrillic());
        InterExtraLight = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(InterExtraLight_compressed_data, InterExtraLight_compressed_size, 14, 0, io.Fonts->GetGlyphRangesCyrillic());
        InterLight = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(InterLight_compressed_data, InterLight_compressed_size, 16, 0, io.Fonts->GetGlyphRangesCyrillic());
        InterMedium = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(InterMedium_compressed_data, InterMedium_compressed_size, 16, 0, io.Fonts->GetGlyphRangesCyrillic());
        InterRegular = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(InterRegular_compressed_data, InterRegular_compressed_size, 16, 0, io.Fonts->GetGlyphRangesCyrillic());
        InterRegular14 = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(InterRegular_compressed_data, InterRegular_compressed_size, 14, 0, io.Fonts->GetGlyphRangesCyrillic());
        InterSemiBold = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(InterSemiBold_compressed_data, InterSemiBold_compressed_size, 16, 0, io.Fonts->GetGlyphRangesCyrillic());
        InterThin = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(InterThin_compressed_data, InterThin_compressed_size, 14, 0, io.Fonts->GetGlyphRangesCyrillic());

        static const ImWchar IconRanges[] =
        {
            ICON_MIN_FA, ICON_MAX_FA, 0
        };

        ImFontConfig FontAwesomeConfig;
        FontAwesomeConfig.GlyphMinAdvanceX = 25.f * (2.0f / 3.0f);

        FontAwesomeRegular = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(FontAwesomeRegular_compressed_data, FontAwesomeRegular_compressed_size, 25.f * (2.0f / 3.0f), &FontAwesomeConfig, IconRanges);
        FontAwesomeSolid = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(FontAwesomeSolid_compressed_data, FontAwesomeSolid_compressed_size, 25.f * (2.0f / 3.0f), &FontAwesomeConfig, IconRanges);
        FontAwesomeSolidBig = ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(FontAwesomeSolid_compressed_data, FontAwesomeSolid_compressed_size, 30.f * (2.0f / 3.0f), &FontAwesomeConfig, IconRanges);

        D3DX11CreateShaderResourceViewFromMemory(g_pd3dDevice, LogoBytes, sizeof(LogoBytes), NULL, NULL, &Logo, NULL);

        Fonts::Initialize(IDevice);


        InitializeMenu();
    }

    void Interface::InitializeMenu() {
        bIsMenuOpen = true;
        SetWindowLong(hWindow, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT);
        SetForegroundWindow(hWindow);
        SetWindowPos(hWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
    }

    void Interface::UpdateStyle() {

        ImGuiStyle* Style = &ImGui::GetStyle();
        ImGuiIO io = ImGui::GetIO();
        Style->WindowRounding = 7;
        Style->WindowBorderSize = 1;
        Style->WindowPadding = ImVec2(0, 0);
        Style->WindowShadowSize = 0;
        Style->ScrollbarSize = 3;
        Style->ScrollbarRounding = 0;
        Style->PopupRounding = 5;
       

        Style->Colors[ImGuiCol_Separator] = ImColor(0, 0, 0, 0);
        Style->Colors[ImGuiCol_SeparatorActive] = ImColor(0, 0, 0, 0);
        Style->Colors[ImGuiCol_SeparatorHovered] = ImColor(0, 0, 0, 0);
        Style->Colors[ImGuiCol_ResizeGrip] = ImColor(0, 0, 0, 0);
        Style->Colors[ImGuiCol_ResizeGripActive] = ImColor(0, 0, 0, 0);
        Style->Colors[ImGuiCol_ResizeGripHovered] = ImColor(0, 0, 0, 0);
        Style->Colors[ImGuiCol_PopupBg] = ImColor(14, 14, 14);

        Style->Colors[ImGuiCol_ScrollbarBg] = ImColor(0, 0, 0, 0);
        Style->Colors[ImGuiCol_ScrollbarGrab] = ImColor(130, 1, 6);
        Style->Colors[ImGuiCol_ScrollbarGrabActive] = ImColor(130, 1, 6);
        Style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImColor(130, 1, 6);

        Style->Colors[ImGuiCol_WindowBg] = ImColor(14, 14, 14);
        Style->Colors[ImGuiCol_Border] = ImColor(24, 23, 25);
    }

    void Interface::RenderGui()
    {
        if (!bIsMenuOpen) return;

        ImGui::SetNextWindowSize(ImVec2(560, 410));
        ImGui::Begin(("Menu"), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            ImDrawList* DrawList = ImGui::GetWindowDrawList();
            ImVec2 Pos = ImGui::GetWindowPos();
            ImVec2 Size = ImGui::GetWindowSize();

            DrawList->AddRectFilled(Pos, Pos + ImVec2(80, Size.y), ImColor(16, 16, 16), ImGui::GetStyle().WindowRounding, ImDrawFlags_RoundCornersLeft);
            DrawList->AddLine(Pos + ImVec2(80, 0), Pos + ImVec2(80, Size.y), ImGui::GetColorU32(ImGuiCol_Border));

            DrawList->AddImage(Logo, Pos + ImVec2(10, 15), Pos + ImVec2(80 - 10, 80 - 10));

            static bool NewTabLower = false;

            ImGui::BeginChild("LeftChild", ImVec2(80, Size.y));
            {
                ImGui::SetCursorPos(ImVec2(20, 80));
                ImGui::BeginGroup();
                {
                    if (ImGui::Tab("Aimbot", ICON_FA_MOUSE, CurrentTab == 0))
                    {
                        OldCurrentTab = CurrentTab;
                        CurrentTab = 0;
                        NewTabLower = (CurrentTab > OldCurrentTab);
                    }
                    if (ImGui::Tab("Visuals", ICON_FA_EYE, CurrentTab == 1))
                    {
                        OldCurrentTab = CurrentTab;
                        CurrentTab = 1;
                        NewTabLower = (CurrentTab > OldCurrentTab);
                    }
                    if (ImGui::Tab("Players", ICON_FA_USERS, CurrentTab == 4))
                    {
                        OldCurrentTab = CurrentTab;
                        CurrentTab = 4;
                        NewTabLower = (CurrentTab > OldCurrentTab);
                    }
                    if (ImGui::Tab("Settings", ICON_FA_FILE, CurrentTab == 5))
                    {
                        OldCurrentTab = CurrentTab;
                        CurrentTab = 5;
                        NewTabLower = (CurrentTab > OldCurrentTab);
                    }
                }
                ImGui::EndGroup();
            }
            ImGui::EndChild();

            static float AimBotChildAnim = 0;
            static float VisualsChildAnim = 0;
            static float MiscChildAnim = 0;
            static float VehiclesChildAnim = 0;
            static float PlayersChildAnim = 0;
            static float SettingsChildAnim = 0;

            AimBotChildAnim = ImLerp(AimBotChildAnim, CurrentTab == 0 ? 0.f : Size.y, ImGui::GetIO().DeltaTime * 10.f);
            VisualsChildAnim = ImLerp(VisualsChildAnim, CurrentTab == 1 ? 0.f : Size.y, ImGui::GetIO().DeltaTime * 10.f);
            MiscChildAnim = ImLerp(MiscChildAnim, CurrentTab == 2 ? 0.f : Size.y, ImGui::GetIO().DeltaTime * 10.f);
            VehiclesChildAnim = ImLerp(VehiclesChildAnim, CurrentTab == 3 ? 0.f : Size.y, ImGui::GetIO().DeltaTime * 10.f);
            PlayersChildAnim = ImLerp(PlayersChildAnim, CurrentTab == 4 ? 0.f : Size.y, ImGui::GetIO().DeltaTime * 10.f);
            SettingsChildAnim = ImLerp(SettingsChildAnim, CurrentTab == 5 ? 0.f : Size.y, ImGui::GetIO().DeltaTime * 10.f);

            ImGui::SetCursorPos(ImVec2(80, AimBotChildAnim));
            ImGui::BeginChild("AimBotMainChild", ImVec2(Size.x - 80, Size.y));
            {
                AimbotTab();
            }
            ImGui::EndChild();

            ImGui::SetCursorPos(ImVec2(80, VisualsChildAnim));
            ImGui::BeginChild("VisualsMainChild", ImVec2(Size.x - 80, Size.y));
            {
                VisualsTab();
            }
            ImGui::EndChild();
             
            ImGui::SetCursorPos(ImVec2(80, PlayersChildAnim));
            ImGui::BeginChild("PlayersMainChild", ImVec2(Size.x - 80, Size.y));
            {
                PlayersTab();
            }
            ImGui::EndChild();
            ImGui::SetCursorPos(ImVec2(80, SettingsChildAnim));
            ImGui::BeginChild("SettingsMainChild", ImVec2(Size.x - 80, Size.y));
            {
                SettingsTab();
            }
            ImGui::EndChild();
        }
        ImGui::End();
            
    }

    void Interface::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        switch (uMsg) {
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                ResizeWidht = (UINT)LOWORD(lParam);
                ResizeHeight = (UINT)HIWORD(lParam);
            }
            break;
        }

        if (bIsMenuOpen) {
            ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
        }
    }

    void Interface::HandleMenuKey()
    {
        static bool MenuKeyDown = false;
        if (GetAsyncKeyState(g_Globals.General.MenuKey) & 0x8000)
        {
            if (!MenuKeyDown)
            {
                MenuKeyDown = true;
                bIsMenuOpen = !bIsMenuOpen;

                if (bIsMenuOpen) {
                    SetWindowLong(hWindow, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
                    SetForegroundWindow(hWindow);
                }
                else {
                    SetWindowLong(hWindow, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE);
                    SetForegroundWindow(hTargetWindow);
                }
                SetWindowPos(hWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
            }
        }
        else {
            MenuKeyDown = false;
        }
    }

    void Interface::ShutDown() {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        Overlay::ShutDown();
    }
}