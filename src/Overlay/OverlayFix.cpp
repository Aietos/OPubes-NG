#include "Overlay/OverlayFix.h"

#include "Pubes/Pubes.h"

namespace {
    constexpr RE::FormID PseudoArmorID{0x806};
    constexpr std::string_view PluginName{"OPubes.esp"};

    // Same wait OCum uses between equipping its pseudo armor and unequipping it, so the body attaches in between
    constexpr auto UnequipDelay{100ms};

    // Racemenu rebuilds the overlays of the actors that loaded with the save in tasks it queues on TESLoadGameEvent,
    // a second later they've run
    constexpr auto LoadDelay{1s};

    // How often it checks again if a menu still pauses the game
    constexpr auto PausedRetryDelay{100ms};

    RE::TESObjectARMO* g_pseudoArmor{nullptr};

    // Actors that have OPubesNude32 equipped right now, so they aren't fixed twice at once
    // Only used from tasks, which run on the main thread
    std::unordered_set<RE::FormID> g_fixing;

    // While a menu pauses the game, like the console or looting a container, actors don't update, so the models of
    // what they equip or unequip aren't attached until it closes. Swapping the body during one leaves the actor without
    // a body for as long as the menu is open, so the swaps wait until the game isn't paused
    bool IsGamePaused() {
        auto* const ui{RE::UI::GetSingleton()};
        return ui && ui->GameIsPaused();
    }

    // Runs the task on the main thread once no menu pauses the game
    void RunWhenUnpaused(std::function<void()> a_task) {
        SKSE::GetTaskInterface()->AddTask([task = std::move(a_task)] {
            if (!IsGamePaused()) {
                task();
                return;
            }

            std::thread([task] {
                std::this_thread::sleep_for(PausedRetryDelay);
                RunWhenUnpaused(task);
            }).detach();
        });
    }

    // Racemenu builds all the body overlays together, so the first one is there if they were built
    bool HasBodyOverlays(RE::Actor* a_actor) {
        static const RE::BSFixedString firstBodyOverlay{"Body [Ovl0]"};

        auto* const root{a_actor->Get3D(false)};
        return root && root->GetObjectByName(firstBodyOverlay);
    }

    // Dressed actors don't need fixing, Racemenu's rebuild finds the body in the armor they wear. Equipping
    // OPubesNude32 on them would also take off their outfit without putting it back
    bool NeedsFix(RE::Actor* a_actor) {
        // OPubes doesn't give pubes to beast races
        return g_pseudoArmor && SKEE::g_overlayInterface && !a_actor->IsPlayerRef() && a_actor->Is3DLoaded() &&
               !Pubes::IsBeastRace(a_actor) && !g_fixing.contains(a_actor->GetFormID()) &&
               SKEE::g_overlayInterface->HasOverlays(a_actor) && !OverlayFix::HasBodyArmor(a_actor) &&
               !HasBodyOverlays(a_actor);
    }

    // Attaching the body again makes Racemenu build the overlays as it's attached, which doesn't have the bug
    void Fix(RE::Actor* a_actor) {
        const auto formID{a_actor->GetFormID()};
        g_fixing.insert(formID);

        logger::info("Fixing the missing overlays of {} ({:08X}) with OPubesNude32", a_actor->GetDisplayFullName(),
                     formID);

        a_actor->AddObjectToContainer(g_pseudoArmor, nullptr, 1, nullptr);
        RE::ActorEquipManager::GetSingleton()->EquipObject(a_actor, g_pseudoArmor, nullptr, 1, nullptr, false, true,
                                                           false, true);

        std::thread([handle = a_actor->GetHandle(), formID] {
            std::this_thread::sleep_for(UnequipDelay);

            // If a menu opened in between, the actor keeps OPubesNude32 and its body until it closes
            RunWhenUnpaused([handle, formID] {
                g_fixing.erase(formID);

                const auto actor{handle.get()};
                if (!actor) return;

                RE::ActorEquipManager::GetSingleton()->UnequipObject(actor.get(), g_pseudoArmor, nullptr, 1, nullptr,
                                                                     false, true, false, true);
                actor->RemoveItem(g_pseudoArmor, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);
            });
        }).detach();
    }
}  // namespace

void OverlayFix::Init() {
    g_pseudoArmor = RE::TESDataHandler::GetSingleton()->LookupForm<RE::TESObjectARMO>(PseudoArmorID, PluginName);

    if (g_pseudoArmor) {
        logger::info("Found OPubesNude32, missing overlays on actors without body armor will be fixed");
    } else {
        logger::critical("Couldn't find OPubesNude32 ({:#x}) in {}, missing overlays won't be fixed", PseudoArmorID,
                         PluginName);
    }
}

bool OverlayFix::IsPseudoArmor(const RE::TESObjectARMO* a_armor) { return a_armor && a_armor == g_pseudoArmor; }

bool OverlayFix::HasBodyArmor(RE::Actor* a_actor) {
    if (const auto* const changes{a_actor->GetInventoryChanges(true)}; changes && changes->entryList) {
        for (auto* const entry : *changes->entryList) {
            if (!entry || !entry->object || !entry->IsWorn()) continue;

            if (const auto* const armor{entry->object->As<RE::TESObjectARMO>()};
                armor && armor->HasPartOf(RE::BGSBipedObjectForm::BipedObjectSlot::kBody)) {
                return true;
            }
        }
    }
    return false;
}

void OverlayFix::FixMissingOverlays(RE::Actor* a_actor) {
    if (!a_actor) return;

    RunWhenUnpaused([handle = a_actor->GetHandle()] {
        if (const auto actor{handle.get()}; actor && NeedsFix(actor.get())) {
            Fix(actor.get());
        }
    });
}

void OverlayFix::FixLoadedActors() {
    std::thread([] {
        std::this_thread::sleep_for(LoadDelay);

        RunWhenUnpaused([] {
            std::size_t fixed{0};
            const auto fixIfNeeded{[&](RE::Actor* a_actor) {
                if (a_actor && NeedsFix(a_actor)) {
                    Fix(a_actor);
                    ++fixed;
                }
            }};

            auto* const processLists{RE::ProcessLists::GetSingleton()};

            processLists->ForEachHighActor([&](RE::Actor* a_actor) {
                fixIfNeeded(a_actor);
                return RE::BSContainer::ForEachResult::kContinue;
            });

            // For exterior cells with actors further away from the player, but that still have their 3D loaded
            for (const auto& handle : processLists->middleHighActorHandles) {
                if (const auto actor{handle.get()}) {
                    fixIfNeeded(actor.get());
                }
            }

            logger::info("Fixed the missing overlays of {} actors after loading the save", fixed);
        });
    }).detach();
}
