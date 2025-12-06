#include "Visual.hpp"
#include <src/Globals.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <Windows.h>
#include <imgui.h>
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <imgui_internal.h>
#include <src/Fonts/Fonts.hpp>
#include "EspLines\Math\Vector\Vector3.hpp"
#include "NameGun.h"
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>

struct HealthBarState {
    float AnimatedHealth = 1.0f;
    float LastUpdateTime = 0.0f;
};

static bool ReadBoneWorld(uint32_t ent, uintptr_t boneOffset, Vector3& outPos) {
    uint32_t bonePtr, t1, t2, matrix;
    if (!Mem.ReadFast2<uint32_t>(ent + boneOffset, &bonePtr) || !bonePtr) return false;
    if (!Mem.ReadFast2<uint32_t>(bonePtr + 0x8, &t1) || !t1)      return false;
    if (!Mem.ReadFast2<uint32_t>(t1 + 0x8, &t2) || !t2)      return false;
    if (!Mem.ReadFast2<uint32_t>(t2 + 0x20, &matrix) || !matrix)  return false;
    return Mem.ReadFast2<Vector3>(matrix + 0x80, &outPos);
}

static bool W2S_Valid(const Vector3& w, ImVec2& o) {
    o = W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, w,
        g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
    return !(o.x < 1 || o.y < 1);
}

struct LocalSkeletonDrawer {
    ImDrawList* list;
    ImU32 color;
    float th;

    inline void Bone(const ImVec2& a, const ImVec2& b) const {
        list->AddLine(a, b, color, th);
    }
    inline void Head(const ImVec2& c, float r) const {
        list->AddCircle(c, r, color, 0, th);
    }
};

static std::unordered_map<int, HealthBarState> healthBarStates;

void DrawVerticalHealthBar(int entityId, short CurrentHealth, short MaxHealth, ImVec2 Position, float TotalHeight) {
    if (MaxHealth <= 0) return;

    ImDrawList* DrawList = ImGui::GetBackgroundDrawList();

    const float BarWidth = 6.0f;
    const float Rounding = 3.0f;
    const float AnimationSpeed = 15.0f;
    const float BorderThickness = 1.0f;

    float HealthPercentage = (std::max)(0.0f, (std::min)(1.0f, static_cast<float>(CurrentHealth) / MaxHealth));

    static std::unordered_map<int, float> animatedHealthValues;
    float& AnimatedHealth = animatedHealthValues[entityId];
    float deltaTime = ImGui::GetIO().DeltaTime;
    float lerpFactor = (std::min)(1.0f, deltaTime * AnimationSpeed);
    AnimatedHealth = ImLerp(AnimatedHealth, HealthPercentage, lerpFactor);

    ImVec2 innerPos = ImVec2(Position.x + BorderThickness, Position.y + BorderThickness);
    float innerWidth = BarWidth - 2 * BorderThickness;
    float innerHeight = TotalHeight - 2 * BorderThickness;
    float FilledBarHeight = innerHeight * AnimatedHealth;

    DrawList->AddRectFilled(
        Position,
        ImVec2(Position.x + BarWidth, Position.y + TotalHeight),
        IM_COL32(40, 40, 40, 130),
        Rounding
    );

    ImU32 HealthBarColor;
    if (HealthPercentage > 0.6f) {
        HealthBarColor = IM_COL32(50, 255, 50, 255);
    }
    else if (HealthPercentage > 0.3f) {
        HealthBarColor = IM_COL32(255, 255, 50, 255);
    }
    else {
        HealthBarColor = IM_COL32(255, 50, 50, 255);
    }

    if (FilledBarHeight > 0.0f) {
        DrawList->AddRectFilled(
            ImVec2(innerPos.x, innerPos.y + innerHeight - FilledBarHeight),
            ImVec2(innerPos.x + innerWidth, innerPos.y + innerHeight),
            HealthBarColor,
            Rounding - BorderThickness,
            ImDrawFlags_RoundCornersBottom
        );
    }

    DrawList->AddRect(
        Position,
        ImVec2(Position.x + BarWidth, Position.y + TotalHeight),
        IM_COL32(60, 60, 60, 255),
        Rounding
    );
}

void DrawHorizontalHealthBar(int entityId, short CurrentHealth, short MaxHealth, ImVec2 Position, float TotalWidth) {
    if (MaxHealth <= 0) return;

    ImDrawList* DrawList = ImGui::GetBackgroundDrawList();

    const float BarHeight = 6.0f;
    const float Rounding = 3.0f;
    const float AnimationSpeed = 15.0f;
    const float BorderThickness = 1.0f;

    float HealthPercentage = (std::max)(0.0f, (std::min)(1.0f, static_cast<float>(CurrentHealth) / MaxHealth));

    static std::unordered_map<int, float> animatedHealthValues;
    float& AnimatedHealth = animatedHealthValues[entityId];
    float deltaTime = ImGui::GetIO().DeltaTime;
    float lerpFactor = (std::min)(1.0f, deltaTime * AnimationSpeed);
    AnimatedHealth = ImLerp(AnimatedHealth, HealthPercentage, lerpFactor);

    ImVec2 innerPos = ImVec2(Position.x + BorderThickness, Position.y + BorderThickness);
    float innerWidth = TotalWidth - 2 * BorderThickness;
    float innerHeight = BarHeight - 2 * BorderThickness;
    float FilledBarWidth = innerWidth * AnimatedHealth;

    DrawList->AddRectFilled(
        Position,
        ImVec2(Position.x + TotalWidth, Position.y + BarHeight),
        IM_COL32(40, 40, 40, 130),
        Rounding
    );

    ImU32 HealthBarColor;
    if (HealthPercentage > 0.6f) {
        HealthBarColor = IM_COL32(50, 255, 50, 255);
    }
    else if (HealthPercentage > 0.3f) {
        HealthBarColor = IM_COL32(255, 255, 50, 255);
    }
    else {
        HealthBarColor = IM_COL32(255, 50, 50, 255);
    }

    if (FilledBarWidth > 0.0f) {
        DrawList->AddRectFilled(
            ImVec2(innerPos.x, innerPos.y),
            ImVec2(innerPos.x + FilledBarWidth, innerPos.y + innerHeight),
            HealthBarColor,
            Rounding - BorderThickness,
            ImDrawFlags_RoundCornersRight
        );
    }

    DrawList->AddRect(
        Position,
        ImVec2(Position.x + TotalWidth, Position.y + BarHeight),
        IM_COL32(60, 60, 60, 255),
        Rounding
    );
}

static void DrawLocalGhostSkeleton() {
    if (!g_Globals.Exploits.Ghost) return;
    if (!g_Globals.EspConfig.Matrix || g_Globals.EspConfig.Width <= 0 || g_Globals.EspConfig.Height <= 0) return;

    const bool held = (GetAsyncKeyState(g_Globals.Exploits.GhostBind) & 0x8000) != 0;
    static bool snapshotTaken = false;

    // snapshot 2D dos ossos
    static ImVec2 sHead, sNeck, sLS, sRS, sLE, sRE, sLW, sRW, sHip, sLA, sRA, sRoot;
    static float sHeadRadius = 8.0f; // tamanho do círculo da cabeça

    if (!held) { // soltou a tecla → limpa o snapshot
        snapshotTaken = false;
        return;
    }

    const uint32_t lp = g_Globals.EspConfig.LocalPlayer;
    if (!lp) return;

    if (!snapshotTaken) {
        // Lê ossos do local player (3D)
        Vector3 wHead{}, wNeck{}, wLS{}, wRS{}, wLE{}, wRE{}, wLW{}, wRW{}, wHip{}, wLA{}, wRA{}, wRoot{};
        bool ok = true;
        ok &= ReadBoneWorld(lp, Offsets::Bones::Head, wHead);
        ok &= ReadBoneWorld(lp, Offsets::Bones::Neck, wNeck);
        ok &= ReadBoneWorld(lp, Offsets::Bones::LeftShoulder, wLS);
        ok &= ReadBoneWorld(lp, Offsets::Bones::RightShoulder, wRS);
        ok &= ReadBoneWorld(lp, Offsets::Bones::LeftElbow, wLE);
        ok &= ReadBoneWorld(lp, Offsets::Bones::RightElbow, wRE);
        ok &= ReadBoneWorld(lp, Offsets::Bones::LeftWrist, wLW);
        ok &= ReadBoneWorld(lp, Offsets::Bones::RightWrist, wRW);
        ok &= ReadBoneWorld(lp, Offsets::Bones::Hip, wHip);
        ok &= ReadBoneWorld(lp, Offsets::Bones::LeftAnkle, wLA);
        ok &= ReadBoneWorld(lp, Offsets::Bones::RightAnkle, wRA);
        ok &= ReadBoneWorld(lp, Offsets::Bones::Root, wRoot);
        if (!ok) return;

        // Converte pra 2D e valida
        if (!W2S_Valid(wHead, sHead)) return;
        if (!W2S_Valid(wNeck, sNeck)) return;
        if (!W2S_Valid(wLS, sLS))   return;
        if (!W2S_Valid(wRS, sRS))   return;
        if (!W2S_Valid(wLE, sLE))   return;
        if (!W2S_Valid(wRE, sRE))   return;
        if (!W2S_Valid(wLW, sLW))   return;
        if (!W2S_Valid(wRW, sRW))   return;
        if (!W2S_Valid(wHip, sHip))  return;
        if (!W2S_Valid(wLA, sLA))   return;
        if (!W2S_Valid(wRA, sRA))   return;
        if (!W2S_Valid(wRoot, sRoot)) return;

        // Raio da cabeça baseado na distância 2D Head-Neck pra ficar proporcional na tela
        float dx = sHead.x - sNeck.x, dy = sHead.y - sNeck.y;
        sHeadRadius = (std::max)(2.0f, (std::min)(16.0f, std::sqrt(dx * dx + dy * dy)));

        snapshotTaken = true;
    }

    // Desenha com AZUL fixo para diferenciar
    LocalSkeletonDrawer sk{
        ImGui::GetBackgroundDrawList(),
        IM_COL32(0, 140, 255, 255), // azul
        1.5f
    };

    // Cabeça
    sk.Head(sHead, sHeadRadius);

    // Tronco
    sk.Bone(sHead, sNeck);
    sk.Bone(sNeck, sHip);

    // Braços
    sk.Bone(sNeck, sLS);
    sk.Bone(sNeck, sRS);
    sk.Bone(sLS, sLE);
    sk.Bone(sRS, sRE);
    sk.Bone(sLE, sLW);
    sk.Bone(sRE, sRW);

    // Pernas
    sk.Bone(sHip, sLA);
    sk.Bone(sHip, sRA);
    sk.Bone(sLA, sRoot); // se preferir até o tornozelo esquerdo
    sk.Bone(sRA, sRoot); // idem direito
}

void DrawCorneredBox(float x, float y, float w, float h, ImColor color, float thickness) {
    auto drawList = ImGui::GetBackgroundDrawList();

    float lineW = w / 3.0f;
    float lineH = h / 3.0f;

    float shadowPad = 2.0f;
    float shadowThickness = 10.0f;
    ImVec2 shadowOffset = ImVec2(1.5f, 1.5f);
    ImU32 boxColor = ImGui::ColorConvertFloat4ToU32(color);

    auto AddShadowLineH = [&](float x_start, float y_pos, float len) {
        drawList->AddLine(ImVec2(x_start, y_pos), ImVec2(x_start + len, y_pos), color, thickness);
        };

    auto AddShadowLineV = [&](float x_pos, float y_start, float len) {
        drawList->AddLine(ImVec2(x_pos, y_start), ImVec2(x_pos, y_start + len), color, thickness);
        };

    AddShadowLineV(x, y - thickness / 2, lineH);
    AddShadowLineH(x - thickness / 2, y, lineW);
    AddShadowLineH(x + w - lineW, y, lineW);
    AddShadowLineV(x + w, y - thickness / 2, lineH);

    AddShadowLineV(x, y + h - lineH, lineH);
    AddShadowLineH(x - thickness / 2, y + h, lineW);

    AddShadowLineH(x + w - lineW, y + h, lineW);
    AddShadowLineV(x + w, y + h - lineH, lineH);
}

void DrawFullBox(float x, float y, float w, float h, ImColor color, float thickness) {
    auto drawList = ImGui::GetBackgroundDrawList();

    float shadowPad = 2.0f;
    float shadowThickness = 10.0f;
    ImVec2 shadowOffset = ImVec2(1.5f, 1.5f);
    ImU32 boxColor = ImGui::ColorConvertFloat4ToU32(color);

    drawList->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), color, 0.0f, ImDrawFlags_None, thickness);
}

void ESP::Players() {
    if (!g_Globals.EspConfig.Matrix) return;

    for (auto& [entityID, player] : g_Globals.EspConfig.Entities) {

        if (player.IsDead || !player.IsKnown) {
            continue;
        }

        float dist = player.Distance;
        if (dist > g_Globals.Visuals.DistanceEsp) {
            continue;
        }

        auto IsOffScreen = [](const ImVec2& pos) { return pos.x < 1 || pos.y < 1; };
        const std::vector<ImVec2> Bones = {
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.Head, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.Neck, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.LeftShoulder, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.RightShoulder, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.LeftElbow, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.RightElbow, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.LeftWrist, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.RightWrist, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.Hip, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.LeftAnkle, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.RightAnkle, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height),
            W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, player.Root, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height)
        };

        if (std::any_of(Bones.begin(), Bones.end(), IsOffScreen)) {
            continue;
        }
        const ImVec2& headPos = Bones[0];
        const ImVec2& neckPos = Bones[1];
        const ImVec2& leftShoulderPos = Bones[2];
        const ImVec2& rightShoulderPos = Bones[3];
        const ImVec2& leftElbowPos = Bones[4];
        const ImVec2& rightElbowPos = Bones[5];
        const ImVec2& leftWristPos = Bones[6];
        const ImVec2& rightWristPos = Bones[7];
        const ImVec2& hipPos = Bones[8];
        const ImVec2& leftAnklePos = Bones[9];
        const ImVec2& rightAnklePos = Bones[10];
        const ImVec2& rootPos = Bones[11];

        Vector3 ankleMidPoint = (player.LeftAnkle + player.RightAnkle) * 0.5f;
        Vector3 hipDirection = (player.LeftAnkle - player.RightAnkle).Normalized(true);
        Vector3 leftHipDirection = hipDirection;
        Vector3 rightHipDirection = -hipDirection;
        float ankleDistance = (player.LeftAnkle - player.RightAnkle).Magnitude(true);
        float baseHipWidth = ankleDistance * g_Globals.Visuals.HipWidthScale;
        Vector3 verticalPositionLeft = Vector3::Lerp(player.LeftAnkle, player.Hip, g_Globals.Visuals.LeftHipHeightOffset);
        Vector3 verticalPositionRight = Vector3::Lerp(player.RightAnkle, player.Hip, g_Globals.Visuals.RightHipHeightOffset);
        Vector3 leftHip = verticalPositionLeft + (leftHipDirection * (baseHipWidth * g_Globals.Visuals.HipWidthOffset));
        Vector3 rightHip = verticalPositionRight + (rightHipDirection * (baseHipWidth * g_Globals.Visuals.HipWidthOffset));
        ImVec2 leftHipPosition = W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, leftHip, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
        ImVec2 rightHipPosition = W2S::WorldToScreenImVec2(g_Globals.EspConfig.ViewMatrix, rightHip, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);

        float boxHeight;
        float boxWidth;

        if (headPos.x != 0 && headPos.y != 0 && rootPos.x != 0 && rootPos.y != 0) {
            boxHeight = fabsf(headPos.y - rootPos.y);
            boxWidth = boxHeight * 0.65f;
        }
        else {
            boxHeight = 50.0f;
            boxWidth = 30.0f;
        }

        if (g_Globals.Visuals.Lines) {
            ImColor snapLineColor = ImColor(
                g_Globals.Visuals.LinesColor[0],
                g_Globals.Visuals.LinesColor[1],
                g_Globals.Visuals.LinesColor[2],
                g_Globals.Visuals.LinesColor[3]
            );

            ImColor KnockedColor = ImColor(
                g_Globals.Visuals.KnockedColor[0],
                g_Globals.Visuals.KnockedColor[1],
                g_Globals.Visuals.KnockedColor[2],
                g_Globals.Visuals.KnockedColor[3]
            );
            ImColor lineColor = player.IsKnocked ? ImColor(KnockedColor) : snapLineColor;
            float thickness = 1.5f;
            float glowThickness = thickness * 1.5f;
            ImColor glowColor = ImColor(lineColor.Value.x * 0.5f, lineColor.Value.y * 0.5f, lineColor.Value.z * 0.5f, 0.3f);

            Vector2 ScreenTop(g_Globals.EspConfig.Width / 2, 0);
            Vector2 ScreenBody(g_Globals.EspConfig.Width / 2, g_Globals.EspConfig.Height);

            auto AddGlowLine = [&](float x1, float y1, float x2, float y2) {
                auto drawList = ImGui::GetBackgroundDrawList();
                drawList->AddLine(
                    ImVec2(x1, y1),
                    ImVec2(x2, y2),
                    lineColor, thickness);
                };

            switch (g_Globals.Visuals.EspLines) {
            case 1:
                AddGlowLine(headPos.x, headPos.y, ScreenTop.X, ScreenTop.Y);
                break;
            case 2:
                AddGlowLine(ScreenBody.X, ScreenBody.Y, headPos.x, headPos.y);
                break;
            }
        }

        if (g_Globals.Exploits.Ghost) {
            DrawLocalGhostSkeleton();
        }


        if (g_Globals.Visuals.Skeleton) {
            ImColor SkeletonColor = ImColor(
                g_Globals.Visuals.SkeletonColor[0],
                g_Globals.Visuals.SkeletonColor[1],
                g_Globals.Visuals.SkeletonColor[2],
                g_Globals.Visuals.SkeletonColor[3]
            );

            struct SkeletonDrawer {
                ImDrawList* drawList;
                ImColor color;
                float thickness;

                void Bone(const ImVec2& from, const ImVec2& to) const {
                    drawList->AddLine(from, to, color, thickness);
                }

                void Head(const ImVec2& center, float radius) const {
                    drawList->AddCircle(center, radius, color, 0, thickness);
                }
            };

            ImColor IsKnockedColor = player.IsKnocked ? ImColor(1.f, 0.f, 0.f, 1.f) : SkeletonColor;

            float headRadius = std::clamp(100.0f / player.Distance, 0.5f, 1.5f) * 2.0f;

            SkeletonDrawer skeleton{
                ImGui::GetBackgroundDrawList(),
                IsKnockedColor,
                1.5f
            };

            skeleton.Head(headPos, headRadius);
            skeleton.Bone(headPos, neckPos);
            skeleton.Bone(neckPos, leftShoulderPos);
            skeleton.Bone(neckPos, rightShoulderPos);
            skeleton.Bone(leftShoulderPos, leftElbowPos);
            skeleton.Bone(rightShoulderPos, rightElbowPos);
            skeleton.Bone(leftElbowPos, leftWristPos);
            skeleton.Bone(rightElbowPos, rightWristPos);
            skeleton.Bone(neckPos, hipPos);
            skeleton.Bone(hipPos, leftHipPosition);
            skeleton.Bone(leftHipPosition, leftAnklePos);

            skeleton.Bone(hipPos, rightHipPosition);
            skeleton.Bone(rightHipPosition, rightAnklePos);

            if (g_Globals.Visuals.Debug) {
                ImGui::GetBackgroundDrawList()->AddCircle(leftHipPosition, 3.0f, IM_COL32(255, 0, 0, 255));
                ImGui::GetBackgroundDrawList()->AddCircle(rightHipPosition, 3.0f, IM_COL32(0, 255, 0, 0));
                ImGui::GetBackgroundDrawList()->AddCircle(hipPos, 3.0f, IM_COL32(0, 0, 255, 255));
            }
        }

        if (g_Globals.Visuals.ESPHealthTEXT) {
            auto drawList = ImGui::GetBackgroundDrawList();
            // --- Health Text ---
            if (g_Globals.Visuals.ESPHealthTEXT) {
                auto drawList = ImGui::GetBackgroundDrawList();
                ImGui::PushFont(FWork::Fonts::InterRegular);
                const char* healthLabel = "Health:";
                ImVec2 labelSize = ImGui::CalcTextSize(healthLabel);

                char healthValue[16];
                snprintf(healthValue, sizeof(healthValue), "%d", player.Health);
                ImVec2 valueSize = ImGui::CalcTextSize(healthValue);

                float totalWidth = labelSize.x + valueSize.x;
                float healthTextOffset = (g_Globals.Visuals.HealthBar && g_Globals.Visuals.players_healthbar == 4) ? 13.5f : 8.0f;
                ImVec2 combinedPosition = ImVec2(rootPos.x - totalWidth / 2, rootPos.y + healthTextOffset - 5);

                ImColor shadowColor = ImColor(0.0f, 0.0f, 0.0f, 0.3f);
                ImColor whiteColor = ImColor(1.0f, 1.0f, 1.0f, 1.0f);

                drawList->AddText(ImVec2(combinedPosition.x + 1, combinedPosition.y + 1), shadowColor, healthLabel);
                drawList->AddText(combinedPosition, whiteColor, healthLabel);

                float healthPercentage = std::clamp(static_cast<float>(player.Health) / 200.0f, 0.0f, 1.0f);
                ImVec4 greenColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
                ImVec4 yellowColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                ImVec4 redColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

                ImVec4 healthBarColor;
                if (healthPercentage > 0.5f) {
                    healthBarColor = ImLerp(greenColor, yellowColor, (1.0f - healthPercentage) * 2.0f);
                }
                else {
                    healthBarColor = ImLerp(yellowColor, redColor, (0.5f - healthPercentage) * 2.0f);
                }
                ImColor health = player.IsKnocked ? ImColor(1.0f, 0.0f, 0.0f, 1.0f) : healthBarColor;

                ImVec2 valuePosition = ImVec2(combinedPosition.x + labelSize.x, combinedPosition.y);

                ImGui::PushFont(FWork::Fonts::InterSemiBold);
                drawList->AddText(ImVec2(valuePosition.x + 1, valuePosition.y + 1), shadowColor, healthValue);
                drawList->AddText(valuePosition, health, healthValue);
                ImGui::PopFont(); // << PopSemiBold
                ImGui::PopFont(); // << PopRegular
            }

            // --- Name + Distance ---
            if (g_Globals.Visuals.Name) {
                ImColor NameColor = ImColor(
                    g_Globals.Visuals.NameColor[0],
                    g_Globals.Visuals.NameColor[1],
                    g_Globals.Visuals.NameColor[2],
                    g_Globals.Visuals.NameColor[3]
                );

                ImColor DistColor = ImColor(
                    g_Globals.Visuals.DistColor[0],
                    g_Globals.Visuals.DistColor[1],
                    g_Globals.Visuals.DistColor[2],
                    g_Globals.Visuals.DistColor[3]
                );

                ImColor shadowColor = ImColor(0.0f, 0.0f, 0.0f, 0.3f);

                ImGui::PushFont(FWork::Fonts::InterSemiBold);
                if (player.Name.empty()) player.Name = "CodeShare";

                float scaleFactor = std::clamp(1.5f - (dist * 0.001f), 0.8f, 1.3f);
                std::string distanceText = g_Globals.Visuals.Distance ? " [" + std::to_string(static_cast<int>(std::round(dist))) + "m]" : "";
                std::string combinedText = player.Name + distanceText;

                ImVec2 combinedSize = ImGui::CalcTextSize(combinedText.c_str());
                ImVec2 textSize = ImGui::CalcTextSize(player.Name.c_str());
                ImVec2 textSizeDistance = ImGui::CalcTextSize(distanceText.c_str());
                float nameOffset = (g_Globals.Visuals.HealthBar && g_Globals.Visuals.players_healthbar == 3) ? 25.0f : 16.0f;
                ImVec2 textPos(headPos.x - (combinedSize.x / 2), headPos.y - nameOffset);
                ImVec2 textPosDistance = ImVec2(textPos.x + textSize.x + 2.2f, textPos.y);

                ImGui::GetBackgroundDrawList()->AddText(ImVec2(textPos.x + 1, textPos.y + 1), shadowColor, player.Name.c_str());
                ImGui::GetBackgroundDrawList()->AddText(textPos, NameColor, player.Name.c_str());

                if (g_Globals.Visuals.Distance) {
                    ImGui::GetBackgroundDrawList()->AddText(ImVec2(textPosDistance.x + 1, textPosDistance.y + 1), shadowColor, distanceText.c_str());
                    ImGui::GetBackgroundDrawList()->AddText(textPosDistance, DistColor, distanceText.c_str());
                }
                ImGui::PopFont(); // << PopSemiBold
            }

            // --- Enemy Count ---
            if (g_Globals.Visuals.Enemy) {
                static int enemyCount = 0;
                enemyCount = std::count_if(g_Globals.EspConfig.Entities.begin(), g_Globals.EspConfig.Entities.end(),
                    [](const auto& pair) { return !pair.second.IsDead && pair.second.IsKnown; });

                static int _lastEnemyCount = 0;
                static std::string _cachedEnemyText;

                if (_lastEnemyCount != enemyCount) {
                    _cachedEnemyText = "Inimigos Detectados: " + std::to_string(enemyCount);
                    _lastEnemyCount = enemyCount;
                }

                ImGui::PushFont(FWork::Fonts::InterSemiBold);
                ImVec2 textSize = ImGui::CalcTextSize(_cachedEnemyText.c_str());

                float textPosY = 70 + ImGui::CalcTextSize("CodeShare").y + 10;
                Vector2 textPos((g_Globals.EspConfig.Width - textSize.x) / 2, textPosY);

                ImColor textColor = ImColor(
                    g_Globals.Visuals.EnemyColor[0],
                    g_Globals.Visuals.EnemyColor[1],
                    g_Globals.Visuals.EnemyColor[2],
                    g_Globals.Visuals.EnemyColor[3]
                );

                ImU32 shadowColor = 0xFF000000;
                ImGui::GetBackgroundDrawList()->AddText(ImVec2(textPos.X + 1, textPos.Y + 1), shadowColor, _cachedEnemyText.c_str());
                ImGui::GetBackgroundDrawList()->AddText(ImVec2(textPos.X, textPos.Y), textColor, _cachedEnemyText.c_str());
                ImGui::PopFont(); // << PopSemiBold
            }


            ImVec2 target;
            float minDistance = FLT_MAX;
            Vector2 screenCenter(g_Globals.EspConfig.Width / 2.0f, g_Globals.EspConfig.Height / 2.0f);

            float distanceToCenter = Vector2::Distance(screenCenter, Vector2(headPos.x, headPos.y));

            if (distanceToCenter < minDistance && distanceToCenter < g_Globals.Visuals.DistanceEsp) {
                minDistance = distanceToCenter;
                target = headPos;
            }
        }
    };
}