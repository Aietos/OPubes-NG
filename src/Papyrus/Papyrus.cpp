#include "Papyrus/Papyrus.h"

#include "Pubes/Pubes.h"

namespace {
    using VM = RE::BSScript::IVirtualMachine;

    // Papyrus calls these from its own threads, the pubes are changed in an SKSE task, where the game changes actors

    void ChangeActorPubes(RE::StaticFunctionTag*, RE::Actor* a_actor) {
        if (!a_actor) return;

        SKSE::GetTaskInterface()->AddTask([handle = a_actor->GetHandle()] {
            if (const auto actor{handle.get()}) {
                Pubes::ChangePubes(actor.get());
            }
        });
    }

    void SetActorPubesType(RE::StaticFunctionTag*, RE::Actor* a_actor, std::string a_type) {
        if (!a_actor) return;

        SKSE::GetTaskInterface()->AddTask([handle = a_actor->GetHandle(), type = std::move(a_type)] {
            if (const auto actor{handle.get()}) {
                Pubes::SetPubesType(actor.get(), type);
            }
        });
    }

    bool BindFunctions(VM* a_vm) {
        constexpr auto obj = "OPubesNative"sv;

#define OPUBES_PAPYRUS_BIND(a_method, ...) \
    a_vm->RegisterFunction(#a_method##sv, obj, a_method __VA_OPT__(, ) __VA_ARGS__)
        OPUBES_PAPYRUS_BIND(ChangeActorPubes);
        OPUBES_PAPYRUS_BIND(SetActorPubesType);
#undef OPUBES_PAPYRUS_BIND

        return true;
    }
}  // namespace

namespace Papyrus {
    void Bind() {
        if (const auto* const papyrus{SKSE::GetPapyrusInterface()}) {
            papyrus->Register(BindFunctions);
        }
    }
}  // namespace Papyrus
