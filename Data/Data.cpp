#include "Data.hpp"
#include <src/Globals.hpp>
#include <EspLines/Memory/Memory.hpp>
#include <EspLines/Offsets.hpp>
#include <EspLines/Math/TMatrix.hpp>
#include <map>
#include <EspLines/Math/Vector/Vector2.hpp>
#include <EspLines/Math/WordToScreen.hpp>
#include <EspLines/Math/AimB.hpp>
#include <EspLines\Aimbot\Leswrite.hpp>
#include <EspLines/Exploits/UpPlayer.hpp>
#include <EspLines/Exploits/DownPlayer.hpp>
#include <EspLines/Exploits/Tpwall.hpp>
#include <EspLines/Exploits/TeleKill.hpp>
#include <EspLines/Aimbot/SilentVerySkidder.hpp>

static Player* lastSilentTarget = nullptr;

namespace FWork {
    void Data::Work() {
        GameContext ctx;

        ctx.currentGame = GetCurrentGame();
        if (!ctx.currentGame) {
            if (!g_Globals.EspConfig.Entities.empty()) {
                g_Globals.EspConfig.Entities.clear();
            }
            Mem.Cache.clear();
            return;
        }

        ctx.currentMatch = GetCurrentMatch(ctx.currentGame);
        if (!ctx.currentMatch) {
            if (!g_Globals.EspConfig.Entities.empty()) {
                g_Globals.EspConfig.Entities.clear();
            }
            Mem.Cache.clear();
            return;
        }

        ctx.localPlayer = Mem.Read<uint32_t>(ctx.currentMatch + Offsets::LocalPlayer);
        if (!ctx.localPlayer || !SetupLocalPlayerAndCamera(ctx.currentMatch)) {
            if (!g_Globals.EspConfig.Entities.empty()) {
                g_Globals.EspConfig.Entities.clear();
            }
            Mem.Cache.clear();
            return;
        }
        ProcessEntities(ctx);

        if (g_Globals.Silent.Enabled && (GetAsyncKeyState(g_Globals.Silent.SilentBind) & 0x8000)) {
            if (g_Globals.EspConfig.Matrix && g_Globals.EspConfig.Width > 0 && g_Globals.EspConfig.Height > 0) {

                Vector2 screenCenter(g_Globals.EspConfig.Width / 2.0f, g_Globals.EspConfig.Height / 2.0f);
                Player* bestTarget = nullptr;
                float bestDistance = FLT_MAX;

                for (auto& pair : g_Globals.EspConfig.Entities) {
                    Player* entity = &pair.second;

                    if (entity->IsDead || entity->IsKnocked || (entity->IsBot && g_Globals.Silent.IgnoreBots))
                        continue;

                    ImVec2 hitBox2D = W2S::WorldToScreenImVec2(
                        g_Globals.EspConfig.ViewMatrix,
                        entity->Head,
                        g_Globals.EspConfig.Width,
                        g_Globals.EspConfig.Height
                    );

                    if (hitBox2D.x < 1 || hitBox2D.y < 1) continue;
                    if (!IsInsideFOV(hitBox2D)) continue;

                    float distanceWorld = Vector3::Distance(g_Globals.EspConfig.MainCamera, entity->Head);
                    if (distanceWorld > g_Globals.AimBot.DistanceAim) continue;

                    float dx = hitBox2D.x - screenCenter.X;
                    float dy = hitBox2D.y - screenCenter.Y;
                    float crosshairDist = sqrtf(dx * dx + dy * dy);

                    if (crosshairDist < bestDistance) {
                        bestDistance = crosshairDist;
                        bestTarget = entity;
                    }
                }

                if (bestTarget) {
                    bool fireCheck = false;
                    if (Mem.ReadFast2<bool>(ctx.localPlayer + Offsets::pomba, &fireCheck) && fireCheck) {
                        uint32_t weaponBase = 0;
                        if (Mem.ReadFast2<uint32_t>(ctx.localPlayer + Offsets::bisteca, &weaponBase) && weaponBase != 0) {
                            Vector3 startPos;
                            if (Mem.ReadFast2<Vector3>(weaponBase + Offsets::arma, &startPos)) {
                                Vector3 targetHitBox = bestTarget->Head;
                                Vector3 direction = (targetHitBox - startPos);
                                direction = Vector3::Normalized(direction);

                                Mem.Write<Vector3>(weaponBase + Offsets::tiro, direction);
                                lastSilentTarget = bestTarget;
                            }
                        }
                    }
                }
            }
        }


        static bool lastAimbotState = false;
        if (g_Globals.AimBot.Enabled != lastAimbotState) {
            if (g_Globals.AimBot.Enabled) {
                veryskidders::StartSilent();
            }
            else {
                veryskidders::StopSilent();
            }
            lastAimbotState = g_Globals.AimBot.Enabled;
        }
        static bool skidders = false;
        if (g_Globals.AimBot.leswrite != skidders) {
            if (skidders) {
                FrameWork::leswritedev::Stop();
            }
            else {
                FrameWork::leswritedev::Work();
            }
            skidders = g_Globals.AimBot.leswrite;
        }
        UpPlayer::UpPlayerr::UpPlayer();
        DownPlayer::DownPlayerr::DownPlayer();
        TpWall::Start();
        TeleKill::Start();
    }

    bool IsInsideFOV(const ImVec2& pos) {
        int centerX = g_Globals.EspConfig.Width / 2;
        int centerY = g_Globals.EspConfig.Height / 2;
        int radius = g_Globals.AimBot.Fov;

        int dx = (int)pos.x - centerX;
        int dy = (int)pos.y - centerY;

        return (dx * dx + dy * dy) <= (radius * radius);
    }

    uint32_t Data::GetCurrentGame() {
        if (Offsets::Il2Cpp == 0) return 0;

#ifdef FFMax
  
        uint32_t staticGameFacade = Mem.Read<uint32_t>(Mem.Read<uint32_t>(Offsets::Il2Cpp + Offsets::InitBase) + Offsets::StaticClass);
        if (!staticGameFacade) return 0;
        return Mem.Read<uint32_t>(staticGameFacade);
#else
        uint32_t baseGameFacade = Mem.Read<uint32_t>(Offsets::Il2Cpp + Offsets::InitBase);
        if (!baseGameFacade) return 0;

        uint32_t gameFacade = Mem.Read<uint32_t>(baseGameFacade);
        if (!gameFacade) return 0;

        uint32_t staticGameFacade = Mem.Read<uint32_t>(gameFacade + Offsets::StaticClass);
        if (!staticGameFacade) return 0;

        return Mem.Read<uint32_t>(staticGameFacade);
#endif
    }
    uint32_t Data::GetCurrentMatch(uint32_t currentGame) {
        if (!currentGame) return 0;

        uint32_t currentMatch = Mem.Read<uint32_t>(currentGame + Offsets::CurrentMatch);
        if (!currentMatch) return 0;

        uint32_t matchStatus = Mem.Read<uint32_t>(currentMatch + Offsets::MatchStatus);
        return (matchStatus == 1) ? currentMatch : 0;
    }

    bool Data::SetupLocalPlayerAndCamera(uint32_t currentMatch) {
        uint32_t localPlayer = Mem.Read<uint32_t>(currentMatch + Offsets::LocalPlayer);
        if (!localPlayer) return false;

        g_Globals.EspConfig.LocalPlayer = localPlayer;

        uint32_t mainTransform = Mem.Read<uint32_t>(localPlayer + Offsets::MainCameraTransform);
        if (!mainTransform) return false;

        Vector3 mainPos;
        TransformUtils::GetPosition(mainTransform, mainPos);
        g_Globals.EspConfig.MainCamera = mainPos;

        uint32_t followCamera = Mem.Read<uint32_t>(localPlayer + Offsets::FollowCamera);
        if (!followCamera) return false;

        uint32_t camera = Mem.Read<uint32_t>(followCamera + Offsets::Camera);
        if (!camera) return false;

        uint32_t cameraBase = Mem.Read<uint32_t>(camera + 0x8);
        if (!cameraBase) return false;

        Matrix4x4 viewMatrix = Mem.Read<Matrix4x4>(cameraBase + Offsets::ViewMatrix);
        g_Globals.EspConfig.Matrix = true;
        g_Globals.EspConfig.ViewMatrix = viewMatrix;

        if (g_Globals.Silent.Enabled) {
            
        }

        if (g_Globals.AimBot.FastReload) {
            uint32_t reload = 0;
            if (Mem.ReadFast2<uint32_t>(localPlayer + Offsets::NoReload, &reload) && reload != 0) {
                Mem.Write<bool>(reload + Offsets::NoReload2, true);
            }
        }
        else {
            uint32_t reload = 0;
            if (Mem.ReadFast2<uint32_t>(localPlayer + Offsets::NoReload, &reload) && reload != 0) {
                Mem.Write<bool>(reload + Offsets::NoReload2, false);
            }
        }
        return true;
    }

    void Data::ProcessEntities(const GameContext& ctx) {
        uint32_t entityDictionary = Mem.Read<uint32_t>(ctx.currentGame + Offsets::DictionaryEntities);
        if (!entityDictionary) return;

        uint32_t entities = Mem.Read<uint32_t>(entityDictionary + 0x14);
        if (!entities) return;

        entities += 0x10;

        uint32_t entitiesCount = Mem.Read<uint32_t>(entityDictionary + 0x18);
        if (entitiesCount != g_Globals.EspConfig.previousCount) {
            g_Globals.EspConfig.previousCount = entitiesCount;
        }

        if (entitiesCount < 1) return;

        Vector3 mainPos = g_Globals.EspConfig.MainCamera;

        for (uint32_t i = 0; i < entitiesCount; ++i) {
            uint32_t entity = Mem.Read<uint32_t>(entities + i * 0x4);

            if (entity == 0 || entity == ctx.localPlayer) continue;

            try {
                Player& player = g_Globals.EspConfig.Entities[entity];

                auto ProcessEntity = [&]() -> bool {

                    uint32_t avatarManager = Mem.Read<uint32_t>(entity + Offsets::AvatarManager);
                    uint32_t avatar = avatarManager ? Mem.Read<uint32_t>(avatarManager + Offsets::Avatar) : 0;
                    uint32_t avatarData = avatar ? Mem.Read<uint32_t>(avatar + Offsets::Avatar_Data) : 0;

                    if (!avatarData) return false;

                    player.IsVisible = Mem.Read<bool>(avatar + Offsets::Avatar_IsVisible);
                    bool isTeam = Mem.Read<bool>(avatarData + Offsets::Avatar_Data_IsTeam);

                    player.IsTeam = isTeam ? Player::Bool3::True : Player::Bool3::False;
                    player.IsKnown = !isTeam;

                    if (!player.IsVisible || player.IsTeam == Player::Bool3::True || !player.IsKnown) {
                        return false;
                    }

                    uint32_t shadowBase = Mem.Read<uint32_t>(entity + Offsets::Player_ShadowBase);
                    player.IsKnocked = shadowBase ? (Mem.Read<int>(shadowBase + Offsets::XPose) == 8) : false;

                    player.IsDead = Mem.Read<bool>(entity + Offsets::Player_IsDead);
                    player.IsBot = Mem.Read<bool>(entity + Offsets::IsClientBot);

                    uint32_t dataPool = Mem.Read<uint32_t>(entity + Offsets::Player_Data);
                    if (!dataPool) return false;
                    uint32_t poolObj = Mem.Read<uint32_t>(dataPool + 0x8);
                    if (!poolObj) return false;
                    uint32_t pool = Mem.Read<uint32_t>(poolObj + 0x10);
                    if (!pool) return false;
                    uint32_t weaponptr = Mem.Read<uint32_t>(poolObj + 0x20);
                    if (!weaponptr) return false;

                    player.Health = Mem.Read<short>(pool + Offsets::Vida);
                    player.WeaponID = Mem.Read<short>(weaponptr + Offsets::Vida);

                    static const std::pair<uint32_t, Vector3 Player::*> boneOffsets[] = {
                        {Offsets::Bones::Head, &Player::Head},
                        {Offsets::Bones::Neck, &Player::Neck},
                        {Offsets::Bones::LeftShoulder, &Player::LeftShoulder},
                        {Offsets::Bones::RightShoulder, &Player::RightShoulder},
                        {Offsets::Bones::LeftElbow, &Player::LeftElbow},
                        {Offsets::Bones::RightElbow, &Player::RightElbow},
                        {Offsets::Bones::LeftWrist, &Player::LeftWrist},
                        {Offsets::Bones::RightWrist, &Player::RightWrist},
                        {Offsets::Bones::Hip, &Player::Hip},
                        {Offsets::Bones::Root, &Player::Root},
                        {Offsets::Bones::LeftAnkle, &Player::LeftAnkle},
                        {Offsets::Bones::RightAnkle, &Player::RightAnkle}
                    };

                    for (const auto& [offset, memberPtr] : boneOffsets) {
                        if (uint32_t boneAddress = 0; Mem.Read(entity + offset, boneAddress)) {
                            TransformUtils::GetNodePosition(boneAddress, player.*memberPtr);
                        }
                    }

                    if (player.Head != Vector3::Zero()) {
                        player.Distance = Vector3::Distance(mainPos, player.Head);
                    }

                    if (uint32_t nameAddr = 0; Mem.Read(entity + Offsets::Player_Name, nameAddr) && nameAddr) {
                        player.Name = Mem.String(nameAddr + 0xC, 128);
                    }

                    player.Address = entity;

                    return true;
                    };

                if (!ProcessEntity()) {
                    g_Globals.EspConfig.Entities.erase(entity);
                    continue;
                }
            }
            catch (...) {
                g_Globals.EspConfig.Entities.erase(entity);
                continue;
            }
        }
    }
}
