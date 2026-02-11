#include "pch.h"
#include "Editor_Logger.h"

shared_ptr<ImGuiSink<mutex>> Editor_Logger::_sink = nullptr;

void Editor_Logger::Initialize()
{
    if (_sink) return;

    if (std::filesystem::exists("Logs") == false)
    {
        std::filesystem::create_directories(L"Logs");
    }

    _sink = make_shared<ImGuiSink<mutex>>();                        // ImGui
    auto msvc_sink = make_shared<spdlog::sinks::msvc_sink_mt>();    // VS 출력창
    auto daily_sink = make_shared<spdlog::sinks::daily_file_sink_mt>("Logs/Log.txt", 0, 0); // 파일

    auto console_sink = make_shared<spdlog::sinks::stdout_color_sink_mt>();

    auto logger = make_shared<spdlog::logger>(
        "EditorLogger", spdlog::sinks_init_list{ _sink, msvc_sink, daily_sink, console_sink });

    spdlog::set_default_logger(logger);

    spdlog::flush_on(spdlog::level::trace);
    spdlog::set_pattern("[%H:%M:%S] %v");
}
