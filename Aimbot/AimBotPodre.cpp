#include "Leswrite.hpp"
#include <iostream>
#include <src/Globals.hpp>

namespace FrameWork {

    std::atomic<bool> leswritedev::isRunning{ false };
    std::thread leswritedev::aimThread;
    std::unordered_map<uintptr_t, uint32_t> leswritedev::patchedMemory;
    std::chrono::steady_clock::time_point leswritedev::lastClickStart = std::chrono::steady_clock::now();
    std::atomic<bool> leswritedev::isClickHeld{ false };
    std::chrono::steady_clock::time_point leswritedev::lastSilentRestore = std::chrono::steady_clock::now();
    std::atomic<bool> leswritedev::alreadyReplaced{ false };
    std::chrono::steady_clock::time_point leswritedev::clickStartTime = std::chrono::steady_clock::time_point();

    void leswritedev::Work() {
        if (isRunning.load()) return;

        isRunning = true;
        aimThread = std::thread(MainLoop);
    }

    void leswritedev::Stop() {
        isRunning = false;
        if (aimThread.joinable()) {
            aimThread.join();
        }
        RestorePatchedMemory();
    }

    bool leswritedev::IsAlive() {
        return isRunning.load() && aimThread.joinable();
    }

    void leswritedev::MainLoop() {
        while (isRunning.load()) {
            try {
                
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSilentRestore).count() >= 3001) {
                    RestorePatchedMemory();
                    lastSilentRestore = now;
                }

                
                if (!g_Globals.AimBot.leswrite ||
                    g_Globals.EspConfig.Width < 1 ||
                    g_Globals.EspConfig.Height < 1 ||
                    !g_Globals.EspConfig.Matrix) {
                    RestorePatchedMemory();
                    alreadyReplaced = false;
                    std::this_thread::sleep_for(std::chrono::milliseconds(11));
                    continue;
                }

              
                Player* target = FindBestTarget();
                if (target == nullptr || target->Address == 0) {
                    alreadyReplaced = false;
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                    continue;
                }

             
                if (!IsMouseDown()) {
                    alreadyReplaced = false;
                    RestorePatchedMemory();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }

            
                if (alreadyReplaced.load()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }

              
                uint32_t headCollider = 0;
                if (Mem.ReadFast2<uint32_t>(target->Address + 0x3F0, &headCollider) && headCollider != 0) {
                  
                    alreadyReplaced = true;

              
                    Mem.Write<uint32_t>(target->Address + 0x50, headCollider);
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            catch (const std::exception& ex) {
                std::cout << "[leswrite ant skidders] error: " << ex.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            catch (...) {
                std::cout << "[leswrite ant skidders] unknown error occurred" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

    
        RestorePatchedMemory();
    }

    void leswritedev::RestorePatchedMemory() {
        for (const auto& kv : patchedMemory) {
            Mem.Write<uint32_t>(kv.first, kv.second);
        }
        patchedMemory.clear();    }

    Player* leswritedev::FindBestTarget() {
        Player* bestTarget = nullptr;
        float closestDistance = FLT_MAX;
        Vector2 screenCenter(g_Globals.EspConfig.Width / 2.0f, g_Globals.EspConfig.Height / 2.0f);

        for (const auto& [id, entity] : g_Globals.EspConfig.Entities) {
            if (entity.Address == 0 || entity.IsDead ||
                (g_Globals.AimBot.IgnoreDowned && entity.IsKnocked)) {
                continue;
            }

            ImVec2 head2D = W2S::WorldToScreenImVec2(
                g_Globals.EspConfig.ViewMatrix,
                entity.Head,
                g_Globals.EspConfig.Width,
                g_Globals.EspConfig.Height
            );

            if (head2D.x < 1 || head2D.y < 1) continue;


            float dist3D = Vector3::Distance(g_Globals.EspConfig.MainCamera, entity.Head);
            if (dist3D > g_Globals.AimBot.distance) continue;


            Vector2 head2DVec(head2D.x, head2D.y);
            float dist2D = Vector2::Distance(screenCenter, head2DVec);

            if (dist2D < closestDistance && dist2D <= g_Globals.AimBot.Fov) {
                closestDistance = dist2D;
                bestTarget = const_cast<Player*>(&entity);
            }
        }

        return bestTarget;
    }

    bool leswritedev::IsMouseDown() {
        return (GetAsyncKeyState(g_Globals.AimBot.AimbotBind) & 0x8000) != 0;
    }
}