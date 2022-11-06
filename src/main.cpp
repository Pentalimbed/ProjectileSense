#include <spdlog/sinks/basic_file_sink.h>

#include "re.h"
#include "config.h"

// #define DBGMSG

bool installLog()
{
    auto path = logger::log_directory();
    if (!path)
        return false;

    *path /= fmt::format(FMT_STRING("{}.log"), SKSE::PluginDeclaration::GetSingleton()->GetName());
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

    auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef DBGMSG
    log->set_level(spdlog::level::info);
    log->flush_on(spdlog::level::info);
#else
    log->set_level(spdlog::level::trace);
    log->flush_on(spdlog::level::trace);
#endif

    spdlog::set_default_logger(std::move(log));
    spdlog::set_pattern("[%H:%M:%S:%e][%5l] %v"s);

    return true;
}

void processMessage(SKSE::MessagingInterface::Message* a_msg)
{
    switch (a_msg->type)
    {
        case SKSE::MessagingInterface::kDataLoaded:
            logger::info("Game: data loaded");

            Config::getSingleton()->init();
            ProjectileHook::Hook();

            break;
        case SKSE::MessagingInterface::kPostLoadGame:
            logger::debug("Game: save loaded");

            if (Config::getSingleton()->loaded())
                Config::getSingleton()->addToggleSpell();

            break;
        default:
            break;
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    installLog();

    auto* plugin  = SKSE::PluginDeclaration::GetSingleton();
    auto  version = plugin->GetVersion();
    logger::info("{} {} is loading...", plugin->GetName(), version);

    SKSE::Init(skse);
    SKSE::AllocTrampoline(14);

    auto messaging = SKSE::GetMessagingInterface();
    if (!messaging->RegisterListener("SKSE", processMessage))
        return false;

    logger::info("{} has finished loading.", plugin->GetName());
    return true;
}
