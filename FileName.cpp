//
//namespace FWork {
//    void Data::Work() {
//        g_Globals.EspConfig.Matrix = false;
//
//        try {
//            if (Offsets::Il2Cpp == 0) return;
//
//            uint32_t baseGameFacade = Mem.Read<uint32_t>(Offsets::Il2Cpp + Offsets::InitBase);
//            if (!baseGameFacade) {
//                Reset();
//                return;
//            }
//
//            uint32_t gameFacade = Mem.Read<uint32_t>(baseGameFacade);
//            if (!gameFacade) {
//                Reset();
//                return;
//            }
//
//            uint32_t staticGameFacade = Mem.Read<uint32_t>(gameFacade + Offsets::StaticClass);
//            if (!staticGameFacade) {
//                Reset();
//                return;
//            }
//
//            uint32_t currentGame = Mem.Read<uint32_t>(staticGameFacade);
//            if (!currentGame) {
//                Reset();
//                return;
//            }
//
//            uint32_t currentMatch = Mem.Read<uint32_t>(currentGame + Offsets::CurrentMatch);
//            if (!currentMatch) {
//                Reset();
//                return;
//            }
//
//            uint32_t localPlayer = Mem.Read<uint32_t>(currentMatch + Offsets::LocalPlayer);
//            if (!localPlayer) {
//                Reset();
//                return;
//            }
//
//            g_Globals.EspConfig.LocalPlayer = localPlayer;
//
//            uint32_t mainTransform = Mem.Read<uint32_t>(localPlayer + Offsets::MainCameraTransform);
//            if (!mainTransform) {
//                Reset();
//                return;
//            }
//
//            Vector3 mainPos;
//            TransformUtils::GetPosition(mainTransform, mainPos);
//            g_Globals.EspConfig.MainCamera = mainPos;
//
//            uint32_t followCamera = Mem.Read<uint32_t>(localPlayer + Offsets::FollowCamera);
//            if (!followCamera) {
//                Reset();
//                return;
//            }
//
//            uint32_t camera = Mem.Read<uint32_t>(followCamera + Offsets::Camera);
//            if (!camera) {
//                Reset();
//                return;
//            }
//
//            uint32_t cameraBase = Mem.Read<uint32_t>(camera + 0x8);
//            if (!cameraBase) {
//                Reset();
//                return;
//            }
//
//            Matrix4x4 viewMatrix = Mem.Read<Matrix4x4>(cameraBase + Offsets::ViewMatrix);
//            g_Globals.EspConfig.Matrix = true;
//            g_Globals.EspConfig.ViewMatrix = viewMatrix;
//
//            uint32_t entityDictionary = Mem.Read<uint32_t>(currentGame + Offsets::DictionaryEntities);
//            if (!entityDictionary) {
//                Reset();
//                return;
//            }
//
//            uint32_t entities = Mem.Read<uint32_t>(entityDictionary + 0x14);
//            if (!entities) {
//                Reset();
//                return;
//            }
//
//            entities += 0x10;
//
//            uint32_t entitiesCount = Mem.Read<uint32_t>(entityDictionary + 0x18);
//            if (entitiesCount != g_Globals.EspConfig.previousCount) {
//                g_Globals.EspConfig.previousCount = entitiesCount;
//            }
//
//            if (entitiesCount < 1) {
//                return;
//            }
//
//            for (uint32_t i = 0; i < entitiesCount; ++i) {
//                uint32_t entity = Mem.Read<uint32_t>(entities + i * 0x4);
//
//                if (entity == 0 || entity == localPlayer) continue;
//
//                try {
//                    auto entry = g_Globals.EspConfig.Entities.find(entity);
//                    Player* entityPtr = nullptr;
//
//                    if (entry != g_Globals.EspConfig.Entities.end()) {
//                        entityPtr = &entry->second;
//                    }
//                    else {
//                        entityPtr = &g_Globals.EspConfig.Entities[entity];
//                    }
//
//                    if (!EntityData(entity, *entityPtr, mainPos)) {
//                        g_Globals.EspConfig.Entities.erase(entity);
//                    }
//                }
//                catch (const std::exception& e) {
//                    std::cerr << "Erro ao coletar dados da entidade " << entity << ": " << e.what() << std::endl;
//                    g_Globals.EspConfig.Entities.erase(entity);
//                }
//            }
//
//            Aim::Aimbot::LegitAimbot();
//        }
//        catch (const std::exception& e) {
//            Reset();
//            std::cerr << "Exception caught: " << e.what() << std::endl;
//        }
//        catch (...) {
//            Reset();
//            std::cerr << "Unknown exception caught!" << std::endl;
//        }
//    }
//
//    bool Data::EntityData(uint32_t entity, Player& player, Vector3& mainPos) {
//        try {
//            uint32_t avatarManager = Mem.Read<uint32_t>(entity + Offsets::AvatarManager);
//            if (avatarManager == 0) {
//                g_Globals.EspConfig.Entities.erase(entity);
//                return false;
//            }
//
//            uint32_t avatar = Mem.Read<uint32_t>(avatarManager + Offsets::Avatar);
//            if (avatar == 0) {
//                g_Globals.EspConfig.Entities.erase(entity);
//                return false;
//            }
//
//            bool isVisible = Mem.Read<bool>(avatar + Offsets::Avatar_IsVisible);
//            if (!isVisible) {
//                g_Globals.EspConfig.Entities.erase(entity);
//                return false;
//            }
//            player.IsVisible = isVisible;
//
//            uint32_t avatarData = Mem.Read<uint32_t>(avatar + Offsets::Avatar_Data);
//            if (avatarData == 0) {
//                g_Globals.EspConfig.Entities.erase(entity);
//                return false;
//            }
//
//            bool isTeam = Mem.Read<bool>(avatarData + Offsets::Avatar_Data_IsTeam);
//            player.IsTeam = isTeam ? Bool3::True : Bool3::False;
//            player.IsKnown = !isTeam;
//
//            if (player.IsTeam == Bool3::True || !player.IsKnown) {
//                return false;
//            }
//
//            uint32_t shadowBase = Mem.Read<uint32_t>(entity + Offsets::Player_ShadowBase);
//            if (shadowBase != 0) {
//                int xpose = Mem.Read<int>(shadowBase + Offsets::XPose);
//                player.IsKnocked = (xpose == 8);
//            }
//
//            player.IsDead = Mem.Read<bool>(entity + Offsets::Player_IsDead);
//            player.IsBot = Mem.Read<bool>(entity + Offsets::IsClientBot);
//
//            uint32_t dataPool = Mem.Read<uint32_t>(entity + Offsets::Player_Data);
//            if (!dataPool) return false;
//            uint32_t poolObj = Mem.Read<uint32_t>(dataPool + 0x8);
//            if (!poolObj) return false;
//            uint32_t pool = Mem.Read<uint32_t>(poolObj + 0x10);
//            if (!pool) return false;
//            uint32_t weaponptr = Mem.Read<uint32_t>(poolObj + 0x20);
//            if (!weaponptr) return false;
//
//            player.Health = Mem.Read<short>(pool + 0x10);
//            player.WeaponID = Mem.Read<short>(weaponptr + 0x10);
//
//            std::map<uint32_t, Vector3*> boneMap = {
//                { (uint32_t)Offsets::Bones::Head, &player.Head },
//                { (uint32_t)Offsets::Bones::Neck, &player.Neck },
//                { (uint32_t)Offsets::Bones::LeftShoulder, &player.LeftShoulder },
//                { (uint32_t)Offsets::Bones::RightShoulder, &player.RightShoulder },
//                { (uint32_t)Offsets::Bones::LeftElbow, &player.LeftElbow },
//                { (uint32_t)Offsets::Bones::RightElbow, &player.RightElbow },
//                { (uint32_t)Offsets::Bones::LeftWrist, &player.LeftWrist },
//                { (uint32_t)Offsets::Bones::RightWrist, &player.RightWrist },
//                { (uint32_t)Offsets::Bones::Hip, &player.Hip },
//                { (uint32_t)Offsets::Bones::Root, &player.Root },
//                { (uint32_t)Offsets::Bones::RootBone, &player.RootBone },
//                { (uint32_t)Offsets::Bones::LeftAnkle, &player.LeftAnkle },
//                { (uint32_t)Offsets::Bones::RightAnkle, &player.RightAnkle },
//            };
//
//            std::vector<uint32_t> bonePointers(boneMap.size(), 0);
//
//            int index = 0;
//            for (const auto& [offset, boneVector] : boneMap) {
//                Mem.Read<uint32_t>(entity + offset, bonePointers[index]);
//                index++;
//            }
//
//            index = 0;
//            for (const auto& [offset, boneVector] : boneMap) {
//                if (bonePointers[index] != 0) {
//                    TransformUtils::GetNodePosition(bonePointers[index], *boneVector);
//                }
//                index++;
//            }
//
//            if (player.Head != Vector3::Zero()) {
//                player.Distance = Vector3::Distance(mainPos, player.Head);
//            }
//
//            if (uint32_t nameAddr = 0; Mem.Read(entity + Offsets::Player_Name, nameAddr) && nameAddr) {
//                player.Name = Mem.String(nameAddr + 0xC, 128, true);
//            }
//
//            player.Address = entity;
//
//            return true;
//        }
//        catch (const std::exception& e) {
//            std::cerr << "Exception caught: " << std::string(e.what()) << std::endl;
//            g_Globals.EspConfig.Entities.erase(entity);
//            return false;
//        }
//    }
//
//    void Data::Reset() {
//        try {
//
            
//
//            Mem.Cache.clear();
//        }
//        catch (const std::exception& e) {
//            std::cerr << "Error clearing game state: " << e.what() << std::endl;
//        }
//        catch (...) {
//            std::cerr << "Unknown error while clearing game state." << std::endl;
//        }
//    }
//}