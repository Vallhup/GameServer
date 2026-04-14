#pragma once

#include <atomic>
#include <chrono>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// ============================================================
//  Log Levels
// ============================================================

enum class LogLevel : uint8_t
{
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Fatal,
    Off,
};

constexpr const char* LogLevelToString(LogLevel level) noexcept
{
    switch (level)
    {
    case LogLevel::Trace: return "TRACE";
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info:  return "INFO ";
    case LogLevel::Warn:  return "WARN ";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Fatal: return "FATAL";
    default:              return "?????";
    }
}

// ============================================================
//  Compile-time gate
// ============================================================

#ifndef FWLOG_MIN_LEVEL
    #ifdef NDEBUG
        #define FWLOG_MIN_LEVEL LogLevel::Info
    #else
        #define FWLOG_MIN_LEVEL LogLevel::Trace
    #endif
#endif

// ============================================================
//  Log Record
// ============================================================

struct LogRecord
{
    LogLevel level{ LogLevel::Info };
    const char* category{ "" };
    const char* file{ "" };
    int line{ 0 };
    uint32_t threadId{ 0 };
    int64_t timestampMs{ 0 };
    char message[512]{};
};

// ============================================================
//  ILogSink  — interface for output destinations
// ============================================================

class ILogSink
{
public:
    virtual ~ILogSink() = default;
    virtual void Write(const LogRecord& record) = 0;
    virtual void Flush() = 0;
};

// ============================================================
//  ConsoleLogSink
// ============================================================

class ConsoleLogSink final : public ILogSink
{
public:
    void Write(const LogRecord& record) override
    {
        FILE* out = (record.level >= LogLevel::Warn) ? stderr : stdout;

        char timeBuf[32]{};
        FormatTimestamp(record.timestampMs, timeBuf, sizeof(timeBuf));

        std::fprintf(out,
            "[%s][%s][%s] %s  (%s:%d)\n",
            timeBuf,
            LogLevelToString(record.level),
            record.category,
            record.message,
            ExtractFileName(record.file),
            record.line);
    }

    void Flush() override
    {
        std::fflush(stdout);
        std::fflush(stderr);
    }

private:
    static const char* ExtractFileName(const char* path) noexcept
    {
        const char* slash = std::strrchr(path, '\\');
        if (slash == nullptr)
            slash = std::strrchr(path, '/');

        return (slash != nullptr) ? slash + 1 : path;
    }

    static void FormatTimestamp(int64_t ms, char* buf, size_t bufSize) noexcept
    {
        const time_t sec = static_cast<time_t>(ms / 1000);
        const int frac = static_cast<int>(ms % 1000);
        struct tm local {};

#ifdef _WIN32
        localtime_s(&local, &sec);
#else
        localtime_r(&sec, &local);
#endif

        const int written = std::snprintf(buf, bufSize,
            "%02d:%02d:%02d.%03d",
            local.tm_hour, local.tm_min, local.tm_sec, frac);

        if (written < 0 || static_cast<size_t>(written) >= bufSize)
            buf[0] = '\0';
    }
};

// ============================================================
//  FileLogSink
// ============================================================

class FileLogSink final : public ILogSink
{
public:
    explicit FileLogSink(const char* filePath)
    {
#ifdef _WIN32
        fopen_s(&_file, filePath, "a");
#else
        _file = std::fopen(filePath, "a");
#endif
    }

    ~FileLogSink() override
    {
        if (_file != nullptr)
        {
            std::fflush(_file);
            std::fclose(_file);
        }
    }

    FileLogSink(const FileLogSink&) = delete;
    FileLogSink& operator=(const FileLogSink&) = delete;

    [[nodiscard]]
    bool IsOpen() const noexcept
    {
        return _file != nullptr;
    }

    void Write(const LogRecord& record) override
    {
        if (_file == nullptr)
            return;

        char timeBuf[32]{};
        FormatTimestamp(record.timestampMs, timeBuf, sizeof(timeBuf));

        std::fprintf(_file,
            "[%s][%s][%s][tid:%u] %s  (%s:%d)\n",
            timeBuf,
            LogLevelToString(record.level),
            record.category,
            record.threadId,
            record.message,
            record.file,
            record.line);
    }

    void Flush() override
    {
        if (_file != nullptr)
            std::fflush(_file);
    }

private:
    FILE* _file{ nullptr };

    static void FormatTimestamp(int64_t ms, char* buf, size_t bufSize) noexcept
    {
        const time_t sec = static_cast<time_t>(ms / 1000);
        const int frac = static_cast<int>(ms % 1000);
        struct tm local {};

#ifdef _WIN32
        localtime_s(&local, &sec);
#else
        localtime_r(&sec, &local);
#endif

        const int written = std::snprintf(buf, bufSize,
            "%04d-%02d-%02d %02d:%02d:%02d.%03d",
            local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
            local.tm_hour, local.tm_min, local.tm_sec, frac);

        if (written < 0 || static_cast<size_t>(written) >= bufSize)
            buf[0] = '\0';
    }
};

// ============================================================
//  FrameworkLog  — central logger (singleton)
// ============================================================

class FrameworkLog final
{
public:
    static FrameworkLog& Instance() noexcept
    {
        static FrameworkLog instance;
        return instance;
    }

    // ----- sink management -----

    void AddSink(ILogSink* sink)
    {
        if (sink == nullptr)
            return;

        std::lock_guard lock{ _sinkMtx };
        _sinks.push_back(sink);
    }

    void RemoveSink(ILogSink* sink) noexcept
    {
        std::lock_guard lock{ _sinkMtx };

        for (auto it = _sinks.begin(); it != _sinks.end(); ++it)
        {
            if (*it == sink)
            {
                _sinks.erase(it);
                return;
            }
        }
    }

    void ClearSinks() noexcept
    {
        std::lock_guard lock{ _sinkMtx };
        _sinks.clear();
    }

    // ----- runtime level control -----

    void SetRuntimeLevel(LogLevel level) noexcept
    {
        _runtimeLevel.store(static_cast<uint8_t>(level),
            std::memory_order_relaxed);
    }

    [[nodiscard]]
    LogLevel GetRuntimeLevel() const noexcept
    {
        return static_cast<LogLevel>(
            _runtimeLevel.load(std::memory_order_relaxed));
    }

    // ----- log entry point -----

    void Log(
        LogLevel level,
        const char* category,
        const char* file,
        int line,
        const char* fmt,
        ...) noexcept
    {
        if (level < GetRuntimeLevel())
            return;

        LogRecord record{};
        record.level = level;
        record.category = category;
        record.file = file;
        record.line = line;
        record.threadId = GetCurrentThreadIdPortable();
        record.timestampMs = GetCurrentTimestampMs();

        va_list args;
        va_start(args, fmt);
        std::vsnprintf(record.message, sizeof(record.message), fmt, args);
        va_end(args);

        std::lock_guard lock{ _sinkMtx };
        for (ILogSink* sink : _sinks)
        {
            sink->Write(record);
        }
    }

    void FlushAll() noexcept
    {
        std::lock_guard lock{ _sinkMtx };
        for (ILogSink* sink : _sinks)
        {
            sink->Flush();
        }
    }

private:
    FrameworkLog() = default;
    ~FrameworkLog() = default;

    FrameworkLog(const FrameworkLog&) = delete;
    FrameworkLog& operator=(const FrameworkLog&) = delete;

    std::mutex _sinkMtx;
    std::vector<ILogSink*> _sinks;
    std::atomic<uint8_t> _runtimeLevel{ static_cast<uint8_t>(LogLevel::Trace) };

    static uint32_t GetCurrentThreadIdPortable() noexcept
    {
#ifdef _WIN32
        // Windows: GetCurrentThreadId() — avoid including windows.h here
        // Use a hash of std::this_thread::get_id() instead for portability.
        const auto id = std::hash<std::thread::id>{}(std::this_thread::get_id());
        return static_cast<uint32_t>(id);
#else
        const auto id = std::hash<std::thread::id>{}(std::this_thread::get_id());
        return static_cast<uint32_t>(id);
#endif
    }

    static int64_t GetCurrentTimestampMs() noexcept
    {
        const auto now = std::chrono::system_clock::now();
        const auto epoch = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
    }
};

// ============================================================
//  Convenience Macros
// ============================================================
//
//  FWLOG_TRACE("Executor", "node %u dispatched", nodeId);
//  FWLOG_ERROR("World",    "runtime not found for scope %u", scopeId);
//
//  Compile-time gate: messages below FWLOG_MIN_LEVEL are completely
//  elided (zero cost) in Release builds.
// ============================================================

#define FWLOG_IMPL(level, category, fmt, ...)                                   \
    do                                                                          \
    {                                                                           \
        if constexpr ((level) >= FWLOG_MIN_LEVEL)                               \
        {                                                                       \
            FrameworkLog::Instance().Log(                                        \
                (level), (category), __FILE__, __LINE__, (fmt), ##__VA_ARGS__); \
        }                                                                       \
    } while (false)

#define FWLOG_TRACE(category, fmt, ...) \
    FWLOG_IMPL(LogLevel::Trace, category, fmt, ##__VA_ARGS__)

#define FWLOG_DEBUG(category, fmt, ...) \
    FWLOG_IMPL(LogLevel::Debug, category, fmt, ##__VA_ARGS__)

#define FWLOG_INFO(category, fmt, ...) \
    FWLOG_IMPL(LogLevel::Info, category, fmt, ##__VA_ARGS__)

#define FWLOG_WARN(category, fmt, ...) \
    FWLOG_IMPL(LogLevel::Warn, category, fmt, ##__VA_ARGS__)

#define FWLOG_ERROR(category, fmt, ...) \
    FWLOG_IMPL(LogLevel::Error, category, fmt, ##__VA_ARGS__)

#define FWLOG_FATAL(category, fmt, ...) \
    FWLOG_IMPL(LogLevel::Fatal, category, fmt, ##__VA_ARGS__)
