#include "Pubes/Pubes.h"

#include "Color/PubesColor.h"
#include "Overlay/OverlayFix.h"
#include "SKEE.h"
#include "Textures/TextureLists.h"

namespace {
    using Lists = TextureLists::Lists;
    using SetVariant = SKEE::IOverrideInterface::SetVariant;
    using GetVariant = SKEE::IOverrideInterface::GetVariant;

    constexpr RE::FormID QuestID{0x802};
    constexpr std::string_view PluginName{"OPubes.esp"};
    constexpr auto ScriptName{"OPubesNGScript"};

    constexpr RE::FormID VampireKeywordID{0xA82BB};
    constexpr RE::FormID BanditFactionID{0x1BCC0};
    constexpr RE::FormID ForswornFactionID{0x43599};

    // Characters without a hair color get very dark brown pubes
    constexpr std::uint32_t DefaultColor{0x2B1D14};

    constexpr auto EmptyTexture{"actors\\character\\overlays\\default.dds"};

    // NiOverride's keys for the node properties OPubes sets
    struct Key {
        enum : std::uint16_t {
            EmissiveColor = 0,
            Glossiness = 2,
            SpecularStrength = 3,
            TintColor = 7,
            Alpha = 8,
            Texture = 9,
        };
    };

    // The index of the diffuse texture, keys that aren't textures use NoIndex
    constexpr std::uint8_t DiffuseIndex{0};
    constexpr std::uint8_t NoIndex{0xFF};

    // The slot of NPCs that rolled shaved, or that didn't have an empty slot
    constexpr std::int32_t NoSlot{-1};

    RE::TESQuest* g_quest{nullptr};
    RE::BGSKeyword* g_vampireKeyword{nullptr};
    RE::TESFaction* g_banditFaction{nullptr};
    RE::TESFaction* g_forswornFaction{nullptr};

    // The NPCs OPubes has given pubes to, with the overlay slot of their pubes, or NoSlot if they're shaved
    std::unordered_map<RE::FormID, std::int32_t> g_actors;

    constexpr std::uint32_t ActorsRecord{'ACTR'};
    constexpr std::uint32_t ActorsVersion{1};

    // The defaults OPubes.esp sets
    struct Settings {
        std::int32_t shavedChance{10};
        std::int32_t normalChance{60};
        std::int32_t stylishChance{5};
        std::int32_t hairyChance{25};
        std::int32_t slot{0};
        bool enableMales{false};
    };

    Settings GetSettings() {
        Settings settings;
        if (!g_quest) return settings;

        auto* const vm{RE::BSScript::Internal::VirtualMachine::GetSingleton()};
        auto* const policy{vm ? vm->GetObjectHandlePolicy() : nullptr};
        if (!policy) return settings;

        RE::BSTSmartPointer<RE::BSScript::Object> object;
        if (!vm->FindBoundObject(policy->GetHandleForObject(RE::TESQuest::FORMTYPE, g_quest), ScriptName, object) ||
            !object) {
            return settings;
        }

        const auto readInt{[&](const char* a_name, std::int32_t& a_value) {
            if (const auto* const property{object->GetProperty(a_name)}; property && property->IsInt()) {
                a_value = property->GetSInt();
            }
        }};
        readInt("ShavedChance", settings.shavedChance);
        readInt("NormalChance", settings.normalChance);
        readInt("StylishChance", settings.stylishChance);
        readInt("HairyChance", settings.hairyChance);
        readInt("SlotToUse", settings.slot);

        if (const auto* const property{object->GetProperty("EnableMales")}; property && property->IsBool()) {
            settings.enableMales = property->GetBool();
        }

        return settings;
    }

    std::int32_t RandomInt(const std::int32_t a_min, const std::int32_t a_max) {
        static std::mt19937 random{std::random_device{}()};
        return std::uniform_int_distribution{a_min, a_max}(random);
    }

    bool ChanceRoll(const std::int32_t a_chance) { return RandomInt(0, 99) < a_chance; }

    std::string RandomTexture(const std::vector<std::string>& a_textures) {
        if (a_textures.empty()) return {};
        return a_textures[RandomInt(0, static_cast<std::int32_t>(a_textures.size()) - 1)];
    }

    bool IsFemale(RE::Actor* a_actor) {
        const auto* const actorBase{a_actor->GetActorBase()};
        return actorBase && actorBase->GetSex() == RE::SEX::kFemale;
    }

    const Lists& GetLists(const bool a_female) {
        return a_female ? TextureLists::GetFemale() : TextureLists::GetMale();
    }

    bool CanHavePubes(RE::Actor* a_actor, const bool a_female, const Settings& a_settings) {
        if (Pubes::IsBeastRace(a_actor)) return false;
        return a_female ? !TextureLists::GetFemale().empty()
                        : a_settings.enableMales && !TextureLists::GetMale().empty();
    }

    bool ContainsOrc(std::string_view a_name) {
        std::string name{a_name};
        std::ranges::transform(name, name.begin(),
                               [](const unsigned char a_char) { return static_cast<char>(std::tolower(a_char)); });
        return name.contains("orc");
    }

    // The list the actor's pubes come from, or nullptr if they're shaved
    const std::vector<std::string>* RollType(RE::Actor* a_actor, const Lists& a_lists, const Settings& a_settings) {
        auto* const actorBase{a_actor->GetActorBase()};
        const auto* const race{actorBase ? actorBase->GetRace() : nullptr};

        if (race && ContainsOrc(race->GetFullName())) {
            if (ChanceRoll(75)) return &a_lists.hairy;
            if (ChanceRoll(50)) return &a_lists.normal;
            if (ChanceRoll(15)) return &a_lists.stylish;
            return nullptr;
        }

        if (race && g_vampireKeyword && race->HasKeyword(g_vampireKeyword)) {
            if (ChanceRoll(50)) return &a_lists.stylish;
            if (ChanceRoll(30)) return nullptr;
            return &a_lists.normal;
        }

        if ((g_banditFaction && a_actor->IsInFaction(g_banditFaction)) ||
            (g_forswornFaction && a_actor->IsInFaction(g_forswornFaction))) {
            if (ChanceRoll(20)) return nullptr;
            return ChanceRoll(80) ? &a_lists.hairy : &a_lists.normal;
        }

        // Most NPCs
        const auto roll{RandomInt(1, 100)};
        auto chance{a_settings.shavedChance};
        if (roll <= chance) return nullptr;
        if (roll <= (chance += a_settings.normalChance)) return &a_lists.normal;
        if (roll <= (chance += a_settings.stylishChance)) return &a_lists.stylish;
        if (roll <= (chance += a_settings.hairyChance)) return &a_lists.hairy;

        // The chances don't add up to 100
        return &a_lists.normal;
    }

    std::string GetNodeName(const std::int32_t a_slot) { return std::format("Body [ovl{}]", a_slot); }

    std::uint32_t GetSlotCount() {
        return SKEE::g_overlayInterface->GetOverlayCount(SKEE::IOverlayInterface::OverlayType::Normal,
                                                         SKEE::IOverlayInterface::OverlayLocation::Body);
    }

    struct IntValue final : SetVariant {
        explicit IntValue(const std::int32_t a_value) : value(a_value) {}
        Type GetType() override { return Type::Int; }
        std::int32_t Int() override { return value; }
        std::int32_t value;
    };

    struct FloatValue final : SetVariant {
        explicit FloatValue(const float a_value) : value(a_value) {}
        Type GetType() override { return Type::Float; }
        float Float() override { return value; }
        float value;
    };

    struct StringValue final : SetVariant {
        explicit StringValue(const char* a_value) : value(a_value) {}
        Type GetType() override { return Type::String; }
        const char* String() override { return value; }
        const char* value;
    };

    struct StringReader final : GetVariant {
        void Int(std::int32_t) override {}
        void Float(float) override {}
        void String(const char* a_value) override { value = a_value ? a_value : ""; }
        void Bool(bool) override {}
        void TextureSet(const RE::BGSTextureSet*) override {}
        std::string value;
    };

    std::string GetSlotTexture(RE::Actor* a_actor, const bool a_female, const std::int32_t a_slot) {
        StringReader reader;
        SKEE::g_overrideInterface->GetNodeOverride(a_actor, a_female, GetNodeName(a_slot).c_str(), Key::Texture,
                                                   DiffuseIndex, reader);
        return reader.value;
    }

    bool IsSlotEmpty(RE::Actor* a_actor, const bool a_female, const std::int32_t a_slot) {
        const auto texture{GetSlotTexture(a_actor, a_female, a_slot)};
        return texture.empty() || PubesColor::NormalizePath(texture) == PubesColor::NormalizePath(EmptyTexture);
    }

    // The slot from the MCM if it's empty, otherwise the first empty one
    std::int32_t FindEmptySlot(RE::Actor* a_actor, const bool a_female, const Settings& a_settings) {
        if (a_settings.slot > 0 && IsSlotEmpty(a_actor, a_female, a_settings.slot)) {
            return a_settings.slot;
        }

        const auto count{static_cast<std::int32_t>(GetSlotCount())};
        for (std::int32_t slot = 0; slot < count; ++slot) {
            if (IsSlotEmpty(a_actor, a_female, slot)) return slot;
        }
        return NoSlot;
    }

    // The slot of the actor's pubes, NoSlot if they're shaved, or nullopt if OPubes didn't give them pubes yet
    // Saves from before the DLL kept track of them only have the pubes in Racemenu's overrides, so actors that aren't
    // tracked are looked for there.
    std::int32_t GetPubesTint(RE::Actor* a_actor, const std::string& a_texture);

    std::optional<std::int32_t> FindSlot(RE::Actor* a_actor, const bool a_female) {
        if (const auto it{g_actors.find(a_actor->GetFormID())}; it != g_actors.end()) {
            return it->second;
        }

        const auto count{static_cast<std::int32_t>(GetSlotCount())};
        for (std::int32_t slot = 0; slot < count; ++slot) {
            if (const auto texture{GetSlotTexture(a_actor, a_female, slot)}; TextureLists::IsPubesTexture(texture)) {
                logger::info("Found the pubes {} ({:08X}) got from an earlier version in slot {}",
                             a_actor->GetDisplayFullName(), a_actor->GetFormID(), slot);
                g_actors.emplace(a_actor->GetFormID(), slot);

                // Earlier versions picked their color differently, sometimes a random gray, so it's picked again
                const auto node{GetNodeName(slot)};
                IntValue tint{GetPubesTint(a_actor, texture)};
                SKEE::g_overrideInterface->AddNodeOverride(a_actor, a_female, node.c_str(), Key::TintColor, NoIndex,
                                                           tint);
                SKEE::g_overrideInterface->SetNodeProperty(a_actor, false, node.c_str(), Key::TintColor, NoIndex, tint,
                                                           true);
                return slot;
            }
        }
        return std::nullopt;
    }

    std::uint32_t ToRGB(const RE::Color& a_color) {
        return static_cast<std::uint32_t>(a_color.red) << 16 | static_cast<std::uint32_t>(a_color.green) << 8 |
               a_color.blue;
    }

    // The hair color with twice the brightness (HSV value), at most full brightness
    std::uint32_t GetLightColor(const RE::Color& a_color) {
        const auto brightest{std::max({a_color.red, a_color.green, a_color.blue})};
        if (brightest == 0) return 0;

        const float scale{std::min(2.0f, 255.0f / brightest)};
        const auto lighten{[&](const std::uint8_t a_channel) {
            return static_cast<std::uint32_t>(std::lround(std::min(255.0f, a_channel * scale)));
        }};
        return lighten(a_color.red) << 16 | lighten(a_color.green) << 8 | lighten(a_color.blue);
    }

    std::int32_t GetPubesTint(RE::Actor* a_actor, const std::string& a_texture) {
        const auto* const actorBase{a_actor->GetActorBase()};
        const auto* const hairColor{actorBase && actorBase->headRelatedData ? actorBase->headRelatedData->hairColor
                                                                            : nullptr};

        // The color the hair shows in game, its texture tinted by the hair color
        std::int32_t target{PubesColor::GetHairColor(a_actor)};

        // The hair or its texture wasn't found, so the hair color is the closest
        if (target < 0) {
            target = static_cast<std::int32_t>(hairColor ? ToRGB(hairColor->color) : DefaultColor);
        }

        if (const auto tint{PubesColor::GetTintForColor(a_texture, static_cast<std::uint32_t>(target))}; tint >= 0) {
            return tint;
        }

        // The texture couldn't be read, so its color isn't known
        if (hairColor) {
            return static_cast<std::int32_t>(GetLightColor(hairColor->color));
        }

        // Without the texture's color, using the color as the tint is the closest to it
        return target;
    }

    // NPCs only get overlay nodes, which are saved with them, once they're added. Adding them rebuilds every overlay,
    // so it's only done once, and doesn't find the body of naked actors, which OverlayFix then attaches again
    void AddOverlays(RE::Actor* a_actor) {
        if (!SKEE::g_overlayInterface->HasOverlays(a_actor)) {
            SKEE::g_overlayInterface->AddOverlays(a_actor, true);
        }

        // For the nodes that were already there
        SKEE::g_overrideInterface->SetNodeProperties(a_actor, true);

        OverlayFix::FixMissingOverlays(a_actor);
    }

    // a_undressing: the actor is taking their clothes off, so the overlays are added when the naked body attaches,
    // see OnBodyAttached. Otherwise, for the cycle key and the MCM, they're added now
    void ApplyPubes(RE::Actor* a_actor, const bool a_female, const std::string& a_texture, const Settings& a_settings,
                    const bool a_undressing) {
        const auto slot{FindEmptySlot(a_actor, a_female, a_settings)};
        if (slot == NoSlot) {
            logger::warn("{} ({:08X}) has no empty body overlay slot for pubes", a_actor->GetDisplayFullName(),
                         a_actor->GetFormID());
            return;
        }

        g_actors[a_actor->GetFormID()] = slot;

        const auto node{GetNodeName(slot)};
        auto* const overrides{SKEE::g_overrideInterface};

        StringValue texture{a_texture.c_str()};
        IntValue tint{GetPubesTint(a_actor, a_texture)};
        IntValue noEmissive{0};
        FloatValue opaque{1.0f};
        FloatValue none{0.0f};

        // Stored, so they're saved and applied to the node whenever Racemenu builds it
        overrides->AddNodeOverride(a_actor, a_female, node.c_str(), Key::Texture, DiffuseIndex, texture);
        overrides->AddNodeOverride(a_actor, a_female, node.c_str(), Key::TintColor, NoIndex, tint);
        overrides->AddNodeOverride(a_actor, a_female, node.c_str(), Key::EmissiveColor, NoIndex, noEmissive);
        overrides->AddNodeOverride(a_actor, a_female, node.c_str(), Key::Alpha, NoIndex, opaque);
        overrides->AddNodeOverride(a_actor, a_female, node.c_str(), Key::Glossiness, NoIndex, none);
        overrides->AddNodeOverride(a_actor, a_female, node.c_str(), Key::SpecularStrength, NoIndex, none);

        if (!a_undressing) {
            AddOverlays(a_actor);
        }

        logger::debug("Gave {} ({:08X}) pubes {} in slot {}", a_actor->GetDisplayFullName(), a_actor->GetFormID(),
                      a_texture, slot);
    }

    void RemovePubes(RE::Actor* a_actor, const bool a_female) {
        const auto slot{FindSlot(a_actor, a_female)};
        if (!slot) return;

        if (*slot != NoSlot) {
            const auto node{GetNodeName(*slot)};
            auto* const overrides{SKEE::g_overrideInterface};

            // Removing the overrides doesn't change what the node shows, so it shows the empty texture first
            StringValue empty{EmptyTexture};
            overrides->SetNodeProperty(a_actor, false, node.c_str(), Key::Texture, DiffuseIndex, empty, true);

            overrides->RemoveNodeOverride(a_actor, a_female, node.c_str(), Key::Texture, DiffuseIndex);
            for (const auto key :
                 {Key::TintColor, Key::EmissiveColor, Key::Alpha, Key::Glossiness, Key::SpecularStrength}) {
                overrides->RemoveNodeOverride(a_actor, a_female, node.c_str(), key, NoIndex);
            }
        }

        g_actors.erase(a_actor->GetFormID());
    }

    bool IsReady() { return SKEE::g_overlayInterface && SKEE::g_overrideInterface; }

    // Gives the actor pubes the first time and adds the overlays, see ApplyPubes for a_undressing
    void Process(RE::Actor* a_actor, std::string a_texture, const bool a_allowPlayer, const bool a_undressing) {
        if (!IsReady()) return;

        const auto settings{GetSettings()};
        const bool female{IsFemale(a_actor)};
        if (!CanHavePubes(a_actor, female, settings)) return;

        if (const auto slot{FindSlot(a_actor, female)}) {
            // This is needed so the pubes stay between save reloads. Shaved actors don't need overlays
            if (*slot != NoSlot && !a_undressing) {
                AddOverlays(a_actor);
            }
            return;
        }

        if (a_actor->IsPlayerRef() && !a_allowPlayer) return;

        // Shaved until they get pubes, so they only roll once
        g_actors[a_actor->GetFormID()] = NoSlot;

        if (a_texture.empty()) {
            if (const auto* const textures{RollType(a_actor, GetLists(female), settings)}) {
                a_texture = RandomTexture(*textures);
            }
        }

        if (!a_texture.empty()) {
            ApplyPubes(a_actor, female, a_texture, settings, a_undressing);
        } else {
            logger::debug("{} ({:08X}) rolled shaved", a_actor->GetDisplayFullName(), a_actor->GetFormID());
        }
    }
}  // namespace

void Pubes::Init() {
    g_quest = RE::TESDataHandler::GetSingleton()->LookupForm<RE::TESQuest>(QuestID, PluginName);
    g_vampireKeyword = RE::TESForm::LookupByID<RE::BGSKeyword>(VampireKeywordID);
    g_banditFaction = RE::TESForm::LookupByID<RE::TESFaction>(BanditFactionID);
    g_forswornFaction = RE::TESForm::LookupByID<RE::TESFaction>(ForswornFactionID);

    if (!g_quest) {
        logger::critical("Couldn't find OPubesNGQuest ({:#x}) in {}, the MCM's settings won't be used", QuestID,
                         PluginName);
    }
}

bool Pubes::IsBeastRace(RE::Actor* a_actor) {
    const auto* const race{a_actor->GetRace()};
    return race && race->HasKeywordString("IsBeastRace");
}

// Outfits are swapped as NPCs load, which looks the same as undressing, so only the pubes are picked here, which is
// cheap. The overlays are added once the naked body actually attaches
void Pubes::OnRemovingClothes(RE::Actor* a_actor) { Process(a_actor, {}, false, true); }

void Pubes::OnBodyAttached(RE::Actor* a_actor, RE::TESObjectARMO* a_armor, RE::TESObjectARMA* a_addon,
                           RE::NiAVObject* a_object, RE::NiNode* a_skeleton, RE::NiNode* a_root) {
    // Actors in Racemenu's list had their overlays built by it already
    if (!IsReady() || SKEE::g_overlayInterface->HasOverlays(a_actor)) return;

    // Only the ones OPubes gave pubes to as they undressed
    const auto it{g_actors.find(a_actor->GetFormID())};
    if (it == g_actors.end() || it->second == NoSlot) return;

    // Adds them to Racemenu's list, its rebuild doesn't find the naked body so it's cheap
    SKEE::g_overlayInterface->AddOverlays(a_actor, true);

    // And builds the overlays on the body that was just attached, the same way Racemenu does for the actors in its
    // list, which works on naked bodies
    SKEE::GetOverlayAttachmentInterface()->OnAttach(a_actor, a_armor, a_addon, a_object, false, a_skeleton, a_root);

    static const RE::BSFixedString firstBodyOverlay{"Body [Ovl0]"};
    if (a_root && a_root->GetObjectByName(firstBodyOverlay)) {
        logger::debug("Built the overlays of {} ({:08X}) on their naked body", a_actor->GetDisplayFullName(),
                      a_actor->GetFormID());
    } else {
        logger::warn("Racemenu didn't build the overlays of {} ({:08X}) on their naked body, their pubes won't show",
                     a_actor->GetDisplayFullName(), a_actor->GetFormID());
    }
}

void Pubes::ChangePubes(RE::Actor* a_actor) {
    if (!IsReady()) return;

    RemovePubes(a_actor, IsFemale(a_actor));
    Process(a_actor, {}, true, false);
}

void Pubes::SetPubesType(RE::Actor* a_actor, const std::string_view a_type) {
    if (!IsReady()) return;

    const bool female{IsFemale(a_actor)};

    if (a_type == "Shaved") {
        RemovePubes(a_actor, female);
        g_actors[a_actor->GetFormID()] = NoSlot;
        return;
    }

    const auto& lists{GetLists(female)};

    std::string texture;

    if (a_type == "Normal") {
        texture = RandomTexture(lists.normal);
    } else if (a_type == "Stylish") {
        texture = RandomTexture(lists.stylish);
    } else if (a_type == "Hairy") {
        texture = RandomTexture(lists.hairy);
    }

    if (texture.empty()) return;

    RemovePubes(a_actor, female);
    Process(a_actor, texture, true, false);
}

void Pubes::Save(SKSE::SerializationInterface* a_save) {
    // Leaves out the actors that don't exist anymore, like the temporary references of NPCs that spawned
    std::vector<std::pair<RE::FormID, std::int32_t>> actors;

    for (const auto& [formID, slot] : g_actors) {
        if (RE::TESForm::LookupByID<RE::Actor>(formID)) {
            actors.emplace_back(formID, slot);
        }
    }

    if (!a_save->OpenRecord(ActorsRecord, ActorsVersion)) {
        logger::error("Couldn't save the actors with pubes");
        return;
    }

    a_save->WriteRecordData(static_cast<std::uint32_t>(actors.size()));
    for (const auto& [formID, slot] : actors) {
        a_save->WriteRecordData(formID);
        a_save->WriteRecordData(slot);
    }
}

void Pubes::Load(SKSE::SerializationInterface* a_load) {
    g_actors.clear();

    std::uint32_t type;
    std::uint32_t version;
    std::uint32_t length;
    while (a_load->GetNextRecordInfo(type, version, length)) {
        if (type != ActorsRecord || version != ActorsVersion) {
            logger::warn("Unknown record {:08X} version {} in the co-save", type, version);
            continue;
        }

        std::uint32_t count{0};
        a_load->ReadRecordData(count);

        for (std::uint32_t i = 0; i < count; ++i) {
            RE::FormID formID{0};
            std::int32_t slot{NoSlot};
            if (a_load->ReadRecordData(formID) != sizeof(formID) || a_load->ReadRecordData(slot) != sizeof(slot)) {
                logger::error("The co-save ended in the middle of the actors with pubes");
                break;
            }

            // Actors from mods that were removed are dropped
            if (a_load->ResolveFormID(formID, formID)) {
                g_actors[formID] = slot;
            }
        }
    }

    logger::info("Loaded {} actors with pubes or shaved from the co-save", g_actors.size());
}

void Pubes::Revert(SKSE::SerializationInterface*) { g_actors.clear(); }
