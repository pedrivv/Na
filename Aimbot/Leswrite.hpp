#pragma once
#include <thread>
#include <atomic>
#include <unordered_map>
#include <chrono>
#include <Windows.h>
#include <EspLines/Player.h>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Memory/Memory.hpp>
#include <src/Globals.hpp>

namespace FrameWork {
    class leswritedev {
    private:
        static std::atomic<bool> isRunning;
        static std::thread aimThread;
        static std::unordered_map<uintptr_t, uint32_t> patchedMemory;
        static std::chrono::steady_clock::time_point lastClickStart;
        static std::atomic<bool> isClickHeld;
        static std::chrono::steady_clock::time_point lastSilentRestore;
        static std::atomic<bool> alreadyReplaced;
        static std::chrono::steady_clock::time_point clickStartTime;


        static void RestorePatchedMemory();
        static Player* FindBestTarget();
        static bool IsMouseDown();
        static void MainLoop();

    public:
        static void Work();
        static void Stop();
        static bool IsAlive();
    };
}
