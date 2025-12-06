#pragma once
#include <cstdint>
#include <EspLines/Math/Vector/Vector3.hpp>
#include <EspLines/Player.h>
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>

struct ImVec2;

struct GameContext {
    uint32_t currentGame;
    uint32_t currentMatch;
    uint32_t localPlayer;
};

namespace FWork {
    class Data {
    public:
        static void Work();
    private:
        static bool EntityData(uint32_t entity, Player& player, Vector3& mainPos);
        static void Reset();
        static uint32_t GetCurrentGame();
        static uint32_t GetCurrentMatch(uint32_t currentGame);
        static bool SetupLocalPlayerAndCamera(uint32_t currentMatch);
        static void ProcessEntities(const GameContext& ctx);

    };
    bool IsInsideFOV(const ImVec2& pos);
}
