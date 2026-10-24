#pragma once

// Racemenu's SKEE interface, in header form so sources aren't included
// Each vtable is declared up to the last function OPubes calls, in the same order as Racemenu's IPluginInterface.h

namespace SKEE {
    class IPluginInterface {
    public:
        IPluginInterface() {};
        virtual ~IPluginInterface() {};

        virtual uint32_t GetVersion() = 0;
        virtual void Revert() = 0;
    };

    class IInterfaceMap {
    public:
        virtual IPluginInterface* QueryInterface(const char* name) = 0;
        virtual bool AddInterface(const char* name, IPluginInterface* pluginInterface) = 0;
        virtual IPluginInterface* RemoveInterface(const char* name) = 0;
    };

    struct InterfaceExchangeMessage {
        enum : uint32_t { kExchangeInterface = 0x9E3779B9 };

        IInterfaceMap* interfaceMap = nullptr;
    };

    class IOverlayInterface : public IPluginInterface {
    public:
        enum class OverlayType { Normal, Spell };
        enum class OverlayLocation { Body, Hand, Feet, Face };

        virtual bool HasOverlays(RE::TESObjectREFR* reference) = 0;
        // Racemenu names the parameter defer, but its implementation treats it as immediate:
        // true builds the overlays right away instead of queueing it as a task
        virtual void AddOverlays(RE::TESObjectREFR* reference, bool immediate = true) = 0;
        virtual void RemoveOverlays(RE::TESObjectREFR* reference, bool defer = true) = 0;
        virtual void RevertOverlays(RE::TESObjectREFR* reference, bool resetDiffuse, bool defer = true) = 0;
        virtual void RevertOverlay(RE::TESObjectREFR* reference, const char* nodeName, uint32_t armorMask,
                                   uint32_t addonMask, bool resetDiffuse, bool defer = true) = 0;
        virtual void EraseOverlays(RE::TESObjectREFR* reference, bool defer = true) = 0;
        virtual void RevertHeadOverlays(RE::TESObjectREFR* reference, bool resetDiffuse, bool defer = true) = 0;
        virtual void RevertHeadOverlay(RE::TESObjectREFR* reference, const char* nodeName, uint32_t partType,
                                       uint32_t shaderType, bool resetDiffuse, bool defer = true) = 0;
        // Version 2
        virtual uint32_t GetOverlayCount(OverlayType type, OverlayLocation location) = 0;
    };

    // Version 2 of the interface, which passes values through these classes instead of Racemenu's own types
    class IOverrideInterface : public IPluginInterface {
    public:
        class GetVariant {
        public:
            virtual void Int(const int32_t i) = 0;
            virtual void Float(const float f) = 0;
            virtual void String(const char* str) = 0;
            virtual void Bool(const bool b) = 0;
            virtual void TextureSet(const RE::BGSTextureSet* textureSet) = 0;
        };

        class SetVariant {
        public:
            enum class Type { None, Int, Float, String, Bool, TextureSet };
            virtual Type GetType() { return Type::None; }
            virtual int32_t Int() { return 0; }
            virtual float Float() { return 0.0f; }
            virtual const char* String() { return nullptr; }
            virtual bool Bool() { return false; }
            virtual RE::BGSTextureSet* TextureSet() { return nullptr; }
        };

        virtual bool HasArmorAddonNode(RE::TESObjectREFR* refr, bool firstPerson, RE::TESObjectARMO* armor,
                                       RE::TESObjectARMA* addon, const char* nodeName, bool debug) = 0;
        virtual bool HasArmorOverride(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor,
                                      RE::TESObjectARMA* addon, const char* nodeName, uint16_t key, uint8_t index) = 0;
        virtual void AddArmorOverride(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor,
                                      RE::TESObjectARMA* addon, const char* nodeName, uint16_t key, uint8_t index,
                                      SetVariant& value) = 0;
        virtual bool GetArmorOverride(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor,
                                      RE::TESObjectARMA* addon, const char* nodeName, uint16_t key, uint8_t index,
                                      GetVariant& visitor) = 0;
        virtual void RemoveArmorOverride(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor,
                                         RE::TESObjectARMA* addon, const char* nodeName, uint16_t key,
                                         uint8_t index) = 0;
        virtual void SetArmorProperties(RE::TESObjectREFR* refr, bool immediate) = 0;
        virtual void SetArmorProperty(RE::TESObjectREFR* refr, bool firstPerson, RE::TESObjectARMO* armor,
                                      RE::TESObjectARMA* addon, const char* nodeName, uint16_t key, uint8_t index,
                                      SetVariant& value, bool immediate) = 0;
        virtual bool GetArmorProperty(RE::TESObjectREFR* refr, bool firstPerson, RE::TESObjectARMO* armor,
                                      RE::TESObjectARMA* addon, const char* nodeName, uint16_t key, uint8_t index,
                                      GetVariant& value) = 0;
        virtual void ApplyArmorOverrides(RE::TESObjectREFR* refr, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon,
                                         RE::NiAVObject* object, bool immediate) = 0;
        virtual void RemoveAllArmorOverrides() = 0;
        virtual void RemoveAllArmorOverridesByReference(RE::TESObjectREFR* reference) = 0;
        virtual void RemoveAllArmorOverridesByArmor(RE::TESObjectREFR* refr, bool isFemale,
                                                    RE::TESObjectARMO* armor) = 0;
        virtual void RemoveAllArmorOverridesByAddon(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor,
                                                    RE::TESObjectARMA* addon) = 0;
        virtual void RemoveAllArmorOverridesByNode(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor,
                                                   RE::TESObjectARMA* addon, const char* nodeName) = 0;

        virtual bool HasNodeOverride(RE::TESObjectREFR* refr, bool isFemale, const char* nodeName, uint16_t key,
                                     uint8_t index) = 0;
        // Only stores the override, it shows once it's applied to the node or the node is built
        virtual void AddNodeOverride(RE::TESObjectREFR* refr, bool isFemale, const char* nodeName, uint16_t key,
                                     uint8_t index, SetVariant& value) = 0;
        virtual bool GetNodeOverride(RE::TESObjectREFR* refr, bool isFemale, const char* nodeName, uint16_t key,
                                     uint8_t index, GetVariant& visitor) = 0;
        virtual void RemoveNodeOverride(RE::TESObjectREFR* refr, bool isFemale, const char* nodeName, uint16_t key,
                                        uint8_t index) = 0;
        // Applies all the stored node overrides to the actor's nodes
        virtual void SetNodeProperties(RE::TESObjectREFR* refr, bool immediate) = 0;
        // Applies a value to the node without storing it
        virtual void SetNodeProperty(RE::TESObjectREFR* refr, bool firstPerson, const char* nodeName, uint16_t key,
                                     uint8_t index, SetVariant& value, bool immediate) = 0;
    };

    // Called after Racemenu's own handlers whenever an armor addon is attached, including the skin's naked body
    class IAddonAttachmentInterface {
    public:
        virtual void OnAttach(RE::TESObjectREFR* refr, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon,
                              RE::NiAVObject* object, bool isFirstPerson, RE::NiNode* skeleton, RE::NiNode* root) = 0;
    };

    class IActorUpdateManager : public IPluginInterface {
    public:
        virtual void AddBodyUpdate(uint32_t formId) = 0;
        virtual void AddTransformUpdate(uint32_t formId) = 0;
        virtual void AddOverlayUpdate(uint32_t formId) = 0;
        virtual void AddNodeOverrideUpdate(uint32_t formId) = 0;
        virtual void AddWeaponOverrideUpdate(uint32_t formId) = 0;
        virtual void AddAddonOverrideUpdate(uint32_t formId) = 0;
        virtual void AddSkinOverrideUpdate(uint32_t formId) = 0;
        virtual void Flush() = 0;
        virtual void AddInterface(IAddonAttachmentInterface* observer) = 0;
    };

    inline IOverlayInterface* g_overlayInterface{nullptr};
    inline IOverrideInterface* g_overrideInterface{nullptr};

    // Racemenu's OverlayInterface inherits IOverlayInterface and then IAddonAttachmentInterface, whose OnAttach builds
    // the overlays on an attached armor or body, the same way it does as they're attached. IOverlayInterface has no
    // data, only its vtable pointer, so the IAddonAttachmentInterface part of the object comes right after it
    inline IAddonAttachmentInterface* GetOverlayAttachmentInterface() {
        return reinterpret_cast<IAddonAttachmentInterface*>(reinterpret_cast<std::uintptr_t>(g_overlayInterface) +
                                                            sizeof(void*));
    }
}  // namespace SKEE
