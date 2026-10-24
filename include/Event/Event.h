#pragma once

#include "SKEE.h"

namespace Event {
    // So the overlays of naked bodies are built as they attach, see Pubes::OnBodyAttached
    void RegisterAttachHandler(SKEE::IActorUpdateManager* a_actorUpdateManager);

    class OPubesEventHandler final : public RE::BSTEventSink<RE::TESEquipEvent>,
                                     public RE::BSTEventSink<RE::TESLoadGameEvent>,
                                     public RE::BSTEventSink<RE::TESObjectLoadedEvent> {
    public:
        static OPubesEventHandler* GetSingleton() { return &singleton; }
        static void Register();

        OPubesEventHandler(OPubesEventHandler&&) = delete;
        OPubesEventHandler(const OPubesEventHandler&) = delete;

        OPubesEventHandler& operator=(OPubesEventHandler&&) = delete;
        OPubesEventHandler& operator=(const OPubesEventHandler&) = delete;

    private:
        static OPubesEventHandler singleton;

        RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* a_event,
                                              RE::BSTEventSource<RE::TESEquipEvent>*) override;

        RE::BSEventNotifyControl ProcessEvent(const RE::TESLoadGameEvent* a_event,
                                              RE::BSTEventSource<RE::TESLoadGameEvent>*) override;

        RE::BSEventNotifyControl ProcessEvent(const RE::TESObjectLoadedEvent* a_event,
                                              RE::BSTEventSource<RE::TESObjectLoadedEvent>*) override;

        OPubesEventHandler() = default;
    };
}  // namespace Event
