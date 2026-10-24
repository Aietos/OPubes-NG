#pragma once

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

// Note: add new modules here before using
#include <chrono>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_set>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>
#include <rapidjson/document.h>
#include <rapidjson/encodedstream.h>
#include <rapidjson/error/en.h>
#include <rapidjson/istreamwrapper.h>

namespace logger = SKSE::log;

using namespace std::literals;

#define DLLEXPORT __declspec(dllexport)
