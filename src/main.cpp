#include "Event/Event.h"
#include "Overlay/OverlayFix.h"
#include "Papyrus/Papyrus.h"
#include "Pubes/Pubes.h"
#include "Textures/TextureLists.h"

namespace {
    void InitializeLogging() {
        // ReSharper disable once CppLocalVariableMayBeConst
        auto path{logger::log_directory()};
        if (!path) {
            SKSE::stl::report_and_fail("Unable to lookup SKSE logs directory.");
        }
        *path /= std::format("{}.log", SKSE::PluginDeclaration::GetSingleton()->GetName());

        auto log{std::make_shared<spdlog::logger>("Global")};
        auto& log_sinks{log->sinks()};

        if (REX::W32::IsDebuggerPresent()) {
            log_sinks.reserve(2);
            const auto msvc_sink{std::make_shared<spdlog::sinks::msvc_sink_mt>()};
            msvc_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [OPubes.dll,%s:%#] %v");
            log_sinks.emplace_back(msvc_sink);
        }
        const auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
        log_sinks.emplace_back(file_sink);

        // Debug has a few lines per NPC that gets pubes, once per save, which is what bug reports need
        log->set_level(spdlog::level::debug);
        log->flush_on(spdlog::level::debug);
        spdlog::set_default_logger(std::move(log));
    }

    // ReSharper disable once CppParameterMayBeConstPtrOrRef
    void SKSEMessageHandler(SKSE::MessagingInterface::Message* a_msg) {
        switch (a_msg->type) {
            // On kPostPostLoad, we can try to fetch the Racemenu interface
            case SKSE::MessagingInterface::kPostPostLoad: {
                SKEE::InterfaceExchangeMessage msg;
                const auto* const intfc{SKSE::GetMessagingInterface()};
                intfc->Dispatch(SKEE::InterfaceExchangeMessage::kExchangeInterface, &msg,
                                sizeof(SKEE::InterfaceExchangeMessage*), "skee");
                if (!msg.interfaceMap) {
                    logger::critical("Couldn't get Racemenu's interface map, OPubes won't give pubes");
                    return;
                }

                // NOLINTBEGIN(*-pro-type-static-cast-downcast)
                auto* const overlayInterface{
                    static_cast<SKEE::IOverlayInterface*>(msg.interfaceMap->QueryInterface("Overlay"))};
                auto* const overrideInterface{
                    static_cast<SKEE::IOverrideInterface*>(msg.interfaceMap->QueryInterface("Override"))};
                // NOLINTEND(*-pro-type-static-cast-downcast)

                // Version 2 of both is needed, for GetOverlayCount and the override functions OPubes declares
                if (!overlayInterface || overlayInterface->GetVersion() < 2 || !overrideInterface ||
                    overrideInterface->GetVersion() < 2) {
                    logger::critical("Racemenu's interfaces are missing or too old, OPubes won't give pubes");
                    return;
                }

                SKEE::g_overlayInterface = overlayInterface;
                SKEE::g_overrideInterface = overrideInterface;

                if (auto* const actorUpdateManager{
                        static_cast<SKEE::IActorUpdateManager*>(msg.interfaceMap->QueryInterface(
                            "ActorUpdateManager"))}) {  // NOLINT(*-pro-type-static-cast-downcast)
                    Event::RegisterAttachHandler(actorUpdateManager);
                } else {
                    logger::critical("Couldn't get Racemenu's ActorUpdateManager, undressing NPCs won't get pubes");
                }
                logger::info("OverlayInterface Version {}, OverrideInterface Version {}",
                             overlayInterface->GetVersion(), overrideInterface->GetVersion());
                return;
            }
            case SKSE::MessagingInterface::kDataLoaded: {
                TextureLists::Load();
                OverlayFix::Init();
                Pubes::Init();
                Event::OPubesEventHandler::Register();
                return;
            }
            default:
                return;
        }
    }
}  // namespace

SKSEPluginLoad(const SKSE::LoadInterface* a_skse) {
    InitializeLogging();

    const auto* const plugin{SKSE::PluginDeclaration::GetSingleton()};
    logger::info("{} {} is loading...", plugin->GetName(), plugin->GetVersion().string("."));

    // We set up our own logger in InitializeLogging
    SKSE::Init(a_skse, {.log = false});

    if (const auto* const message = SKSE::GetMessagingInterface(); !message->RegisterListener(SKSEMessageHandler)) {
        return false;
    }

    Papyrus::Bind();

    const auto* const serialization{SKSE::GetSerializationInterface()};
    serialization->SetUniqueID('OPUB');
    serialization->SetSaveCallback(Pubes::Save);
    serialization->SetLoadCallback(Pubes::Load);
    serialization->SetRevertCallback(Pubes::Revert);

    logger::info("{} has finished loading.", plugin->GetName());

    return true;
}
