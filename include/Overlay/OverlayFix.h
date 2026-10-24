#pragma once

#include "SKEE.h"

// Racemenu builds overlays in two ways: when an armor with the body is attached, which always works, or by rebuilding
// them afterwards, which is what AddOverlays does and what it does for the actors that loaded while a save was loading.
// The rebuild doesn't find the body of actors with nothing in the body slot, so their overlays are missing until the
// body is attached again. OverlayFix attaches it again by equipping and unequipping OPubesNude32
namespace OverlayFix {
    // Looks up OPubesNude32, after the data is loaded
    void Init();

    bool IsPseudoArmor(const RE::TESObjectARMO* a_armor);

    // If the actor wears anything in the body slot, from their inventory, so it's what they wear even if the models
    // haven't changed yet
    bool HasBodyArmor(RE::Actor* a_actor);

    void FixMissingOverlays(RE::Actor* a_actor);

    // Fixes the overlays of the loaded actors after a save loads, once Racemenu has rebuilt them
    void FixLoadedActors();
}  // namespace OverlayFix
