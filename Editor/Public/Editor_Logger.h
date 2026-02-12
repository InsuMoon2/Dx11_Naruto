#pragma once

NS_BEGIN(Editor)

struct LogEntry
{
    LogLevel level;
    string message;
};

template<typename Mutex>
class ImGuiSink : public spdlog::sinks::base_sink<Mutex>
{
public:
    const vector<LogEntry>& GetLogs() const { return _logs; }
    void ClearLogs() { _logs.clear(); }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        LogLevel level = LogLevel::Info;
        if (msg.level == spdlog::level::warn) level = LogLevel::Warning;
        else if (msg.level == spdlog::level::err) level = LogLevel::Error;
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);

        _logs.push_back({ level, fmt::to_string(formatted) });
    }
    void flush_() override {}

private:
    vector<LogEntry> _logs;
};

class Editor_Logger
{
public:
    static void Initialize();
    static shared_ptr<ImGuiSink<mutex>> GetSink() { return _sink; }

private:
    static shared_ptr<ImGuiSink<mutex>> _sink;
};

NS_END
