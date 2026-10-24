#pragma once

// Gives NPCs pubes when they start undressing, picks their type and texture, and colors them like their hair
namespace Pubes {
    // Looks up the forms it uses, after the data is loaded
    void Init();

    bool IsBeastRace(RE::Actor* a_actor);

    void OnRemovingClothes(RE::Actor* a_actor);

    // Called when an actor's naked body attaches, after Racemenu's own handler. Builds the overlays of the actors
    // OPubes gave pubes to as they undressed
    void OnBodyAttached(RE::Actor* a_actor, RE::TESObjectARMO* a_armor, RE::TESObjectARMA* a_addon,
                        RE::NiAVObject* a_object, RE::NiNode* a_skeleton, RE::NiNode* a_root);

    void ChangePubes(RE::Actor* a_actor);

    void SetPubesType(RE::Actor* a_actor, std::string_view a_type);

    // Which NPCs have pubes and in which overlay slot, or are shaved, kept in OPubes' co-save
    void Save(SKSE::SerializationInterface* a_save);
    void Load(SKSE::SerializationInterface* a_load);
    void Revert(SKSE::SerializationInterface* a_revert);
}  // namespace Pubes
