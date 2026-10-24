#include "Event/Event.h"

#include "Color/PubesColor.h"
#include "Overlay/OverlayFix.h"
#include "Pubes/Pubes.h"

constinit Event::OPubesEventHandler Event::OPubesEventHandler::singleton;

namespace {
    // OPubes skips the player, their pubes are set manually
    bool IsOPubesTarget(RE::Actor* a_actor) {
        return !a_actor->IsPlayerRef() && a_actor->HasKeywordString("ActorTypeNPC") && !a_actor->IsChild();
    }

    // Actor::GetInventory copies the actor's whole inventory every time it's called
    // That adds up in equip events, which come in bursts while NPCs load, so this goes through the entries instead
    bool IsWorn(RE::Actor* a_actor, const RE::TESObjectARMO* a_armor) {
        if (const auto* const changes{a_actor->GetInventoryChanges(true)}; changes && changes->entryList) {
            for (auto* const entry : *changes->entryList) {
                if (entry && entry->object == a_armor && entry->IsWorn()) {
                    return true;
                }
            }
        }
        return false;
    }

    // Same slots OBody checks, the outfit's body, chest and pelvis garments
    bool CoversClothingSlot(const RE::TESObjectARMO* a_armor) {
        using BipedObjectSlot = RE::BGSBipedObjectForm::BipedObjectSlot;
        constexpr std::array clothingSlots{BipedObjectSlot::kBody, BipedObjectSlot::kModChestPrimary,
                                           BipedObjectSlot::kModChestSecondary, BipedObjectSlot::kModPelvisPrimary,
                                           BipedObjectSlot::kModPelvisSecondary};

        return std::ranges::any_of(clothingSlots, [&](const auto a_slot) { return a_armor->HasPartOf(a_slot); });
    }

    bool IsRemovingClothes(RE::Actor* a_actor, const RE::TESObjectARMO* a_armor) {
        // When the TES EquipEvent is sent, the inventory isn't updated yet, so the armor being removed is still worn
        return CoversClothingSlot(a_armor) && IsWorn(a_actor, a_armor);
    }

    // Racemenu calls it whenever an armor addon is attached, after its own handlers
    class AttachHandler final : public SKEE::IAddonAttachmentInterface {
    public:
        void OnAttach(RE::TESObjectREFR* a_refr, RE::TESObjectARMO* a_armor, RE::TESObjectARMA* a_addon,
                      RE::NiAVObject* a_object, const bool a_firstPerson, RE::NiNode* a_skeleton,
                      RE::NiNode* a_root) override {
            if (a_firstPerson || !a_refr || !a_armor || !a_addon ||
                !a_addon->HasPartOf(RE::BGSBipedObjectForm::BipedObjectSlot::kBody)) {
                return;
            }

            auto* const actor{a_refr->As<RE::Actor>()};
            if (!actor || !IsOPubesTarget(actor)) return;

            // Something in the body slot means it isn't the naked body, which is the skin's
            if (!OverlayFix::HasBodyArmor(actor)) {
                Pubes::OnBodyAttached(actor, a_armor, a_addon, a_object, a_skeleton, a_root);
            }
        }
    };

    AttachHandler g_attachHandler;
}  // namespace

void Event::RegisterAttachHandler(SKEE::IActorUpdateManager* a_actorUpdateManager) {
    a_actorUpdateManager->AddInterface(&g_attachHandler);
    logger::info("Registered for armor attachments");
}

void Event::OPubesEventHandler::Register() {
    if (auto* const events{RE::ScriptEventSourceHolder::GetSingleton()}) {
        events->AddEventSink<RE::TESEquipEvent>(&singleton);
        events->AddEventSink<RE::TESLoadGameEvent>(&singleton);
        events->AddEventSink<RE::TESObjectLoadedEvent>(&singleton);
        logger::info("Registered for the equip, load game and object loaded events");
    }
}

RE::BSEventNotifyControl Event::OPubesEventHandler::ProcessEvent(const RE::TESEquipEvent* a_event,
                                                                 RE::BSTEventSource<RE::TESEquipEvent>*) {
    if (!a_event || a_event->equipped || !a_event->actor || a_event->baseObject == 0) {
        return RE::BSEventNotifyControl::kContinue;
    }

    auto* const actor{a_event->actor->As<RE::Actor>()};
    const auto* const armor{RE::TESForm::LookupByID<RE::TESObjectARMO>(a_event->baseObject)};

    // OverlayFix taking off OPubesNude32 isn't the actor undressing
    // Actors without their 3D aren't seen undressing, they get their pubes the next time they undress loaded
    if (actor && armor && !OverlayFix::IsPseudoArmor(armor) && IsOPubesTarget(actor) &&
        IsRemovingClothes(actor, armor) && actor->Is3DLoaded()) {
        Pubes::OnRemovingClothes(actor);
    }

    return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl Event::OPubesEventHandler::ProcessEvent(const RE::TESLoadGameEvent* a_event,
                                                                 RE::BSTEventSource<RE::TESLoadGameEvent>*) {
    if (a_event) {
        OverlayFix::FixLoadedActors();
    }

    return RE::BSEventNotifyControl::kContinue;
}

// Only reads the hair texture's color in the background, so it's known by the time the NPC undresses
RE::BSEventNotifyControl Event::OPubesEventHandler::ProcessEvent(const RE::TESObjectLoadedEvent* a_event,
                                                                 RE::BSTEventSource<RE::TESObjectLoadedEvent>*) {
    if (!a_event || !a_event->loaded) return RE::BSEventNotifyControl::kContinue;

    if (auto* const actor{RE::TESForm::LookupByID<RE::Actor>(a_event->formID)}; actor && IsOPubesTarget(actor)) {
        PubesColor::CacheHairTexture(actor);
    }

    return RE::BSEventNotifyControl::kContinue;
}
