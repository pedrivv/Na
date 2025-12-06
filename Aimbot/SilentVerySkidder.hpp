#pragma once

#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <Windows.h>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <EspLines/Player.h>
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Math/WordToScreen.hpp>

namespace veryskidders {

    inline std::atomic<bool> cancel = false;
    inline std::atomic<bool> running = false;
    inline std::thread silentThread;

    inline Vector3 GetHitboxPosition(const Player& entity) {
        switch (g_Globals.AimBot.Hitbox) {
        case 0: return entity.Head;
        case 1: return entity.Neck;
        case 2: return entity.LeftShoulder;
        case 3: return entity.RightShoulder;
        default: return entity.Head;
        }
    }

    inline Player* GetBestTarget() {
        try {
            Player* bestTarget = nullptr;
            float bestMetric = FLT_MAX;

            Vector2 screenCenter(g_Globals.EspConfig.Width / 2.0f, g_Globals.EspConfig.Height / 2.0f);

            for (auto& pair : g_Globals.EspConfig.Entities) {
                try {
                    Player& entity = pair.second;

                    if (!entity.IsKnown || entity.IsDead) continue;
                    if (g_Globals.AimBot.IgnoreDowned && entity.IsKnocked) continue;
                    if (g_Globals.AimBot.IgnoreTrainingBots) {
                        if (!entity.Name.empty()) continue;
                    }

                    Vector3 bonePos = GetHitboxPosition(entity);
                    float worldDist = Vector3::Distance(g_Globals.EspConfig.MainCamera, bonePos);
                    if (worldDist > g_Globals.AimBot.Distance) continue;

                    Vector2 screenPos = W2S::WorldToScreen(g_Globals.EspConfig.ViewMatrix, bonePos, g_Globals.EspConfig.Width, g_Globals.EspConfig.Height);
                    if (screenPos.X < 0 || screenPos.Y < 0) continue;

                    float fovDist = Vector2::Distance(screenCenter, screenPos);
                    if (g_Globals.AimBot.UseFov && fovDist > g_Globals.AimBot.Fov) continue;

                    bool better = false;

                    if (g_Globals.AimBot.PriorityFov) {
                        better = fovDist < bestMetric;
                        if (better) bestMetric = fovDist;
                    }

                    if (!better && g_Globals.AimBot.PriorityHealth) {
                        better = entity.Health < bestMetric;
                        if (better) bestMetric = entity.Health;
                    }

                    if (!better && g_Globals.AimBot.PriorityDistance) {
                        better = worldDist < bestMetric;
                        if (better) bestMetric = worldDist;
                    }

                    if (better) bestTarget = &entity;
                }
                catch (...) {

                    continue;
                }
            }
            return bestTarget;
        }
        catch (...) {

            return nullptr;
        }
    }

    inline void SilentAimAtTarget(Player* target) {
        try {
            if (!target) return;

            Vector3 targetPos = GetHitboxPosition(*target);
            targetPos.Y += 0.1f;

            bool isAiming;
            if (!Mem.ReadFast2<bool>(g_Globals.EspConfig.LocalPlayer + 0x488, &isAiming) || !isAiming) {
                return;
            }

            uint32_t weaponPtr;
            if (!Mem.ReadFast2<uint32_t>(g_Globals.EspConfig.LocalPlayer + 0x854, &weaponPtr) || weaponPtr == 0) {
                return;
            }

            Vector3 shootOrigin;
            if (!Mem.ReadFast2<Vector3>(weaponPtr + 0x38, &shootOrigin)) {
                return;
            }

            Vector3 direction = Vector3::Normalized(targetPos - shootOrigin);
            Mem.Write<Vector3>(weaponPtr + 0x2C, direction);
        }
        catch (...) {

        }
    }

    inline void Silent() {
        try {
            if (!(GetAsyncKeyState(g_Globals.AimBot.SilentBind) & 0x8000)) return;
            if (g_Globals.EspConfig.Width == 0 || g_Globals.EspConfig.Height == 0) return;
            if (!g_Globals.EspConfig.Matrix) return;

            Player* target = GetBestTarget();
            if (target != nullptr) {
                SilentAimAtTarget(target);
            }
        }
        catch (...) {

        }
    }

    inline void StopSilent() {
        cancel = true;
        if (silentThread.joinable()) {
            try {
                silentThread.join();
            }
            catch (...) {

            }
        }
    }

    inline void StartSilent() {
        if (running.load()) return;


        StopSilent();

        cancel = false;
        running = true;

        silentThread = std::thread([] {
            try {
                while (!cancel.load()) {
                    Silent();

                }
            }
            catch (...) {

            }
            running = false;
            });
    }
}