#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string_view>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
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
//
//  Debug  빌드: Trace 이상 모두 포함
//  Release빌드: Info  이상만 포함 (Trace/Debug 코드 완전 제거)
//  배포 오버라이드: /DFWLOG_MIN_LEVEL=LogLevel::Warn
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
    LogLevel    level{ LogLevel::Info };
    const char* category{ "" };
    const char* file{ "" };
    int         line{ 0 };
    uint32_t    shortThreadId{ 0 };
    int64_t     timestampMs{ 0 };
    char        message[1024]{};
};

// ============================================================
//  LogFormat  — 공유 포맷 유틸리티
// ============================================================

namespace LogFormat
{
    inline void FormatTimestamp(int64_t ms, char* buf, size_t bufSize) noexcept
    {
        const time_t sec  = static_cast<time_t>(ms / 1000);
        const int    frac = static_cast<int>(ms % 1000);
        struct tm local{};

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

    inline void FormatDate(int64_t ms, char* buf, size_t bufSize) noexcept
    {
        const time_t sec = static_cast<time_t>(ms / 1000);
        struct tm local{};

#ifdef _WIN32
        localtime_s(&local, &sec);
#else
        localtime_r(&sec, &local);
#endif

        std::snprintf(buf, bufSize, "%04d-%02d-%02d",
            local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);
    }

    inline const char* ExtractFileName(const char* path) noexcept
    {
        std::string_view sv(path);
        const auto pos = sv.find_last_of("/\\");
        return (pos != std::string_view::npos) ? path + pos + 1 : path;
    }

    // 통일 포맷:
    //   YYYY-MM-DD HH:MM:SS.mmm  LEVEL  Category        T01   message
    //   YYYY-MM-DD HH:MM:SS.mmm  ERROR  Category        T01   message  (file.cpp:line)
    inline int FormatRecord(const LogRecord& r, char* buf, size_t bufSize) noexcept
    {
        char timeBuf[32]{};
        FormatTimestamp(r.timestampMs, timeBuf, sizeof(timeBuf));

        if (r.level >= LogLevel::Error)
        {
            return std::snprintf(buf, bufSize,
                "%s  %-5s  %-14s  T%02u   %s  (%s:%d)\n",
                timeBuf, LogLevelToString(r.level),
                r.category, r.shortThreadId,
                r.message, ExtractFileName(r.file), r.line);
        }
        else
        {
            return std::snprintf(buf, bufSize,
                "%s  %-5s  %-14s  T%02u   %s\n",
                timeBuf, LogLevelToString(r.level),
                r.category, r.shortThreadId,
                r.message);
        }
    }

} // namespace LogFormat

// ============================================================
//  ILogSink
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
        char lineBuf[1200]{};
        LogFormat::FormatRecord(record, lineBuf, sizeof(lineBuf));
        std::fputs(lineBuf, out);
    }

    void Flush() override
    {
        std::fflush(stdout);
        std::fflush(stderr);
    }
};

// ============================================================
//  FileLogSink  — 고정 파일 (테스트/단순 용도)
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

    FileLogSink(const FileLogSink&)            = delete;
    FileLogSink& operator=(const FileLogSink&) = delete;

    [[nodiscard]] bool IsOpen() const noexcept { return _file != nullptr; }

    void Write(const LogRecord& record) override
    {
        if (_file == nullptr) return;
        char lineBuf[1200]{};
        LogFormat::FormatRecord(record, lineBuf, sizeof(lineBuf));
        std::fputs(lineBuf, _file);
    }

    void Flush() override
    {
        if (_file != nullptr) std::fflush(_file);
    }

private:
    FILE* _file{ nullptr };
};

// ============================================================
//  RollingFileLogSink  — 날짜 기반 자동 로테이션
// ============================================================

class RollingFileLogSink final : public ILogSink
{
public:
    explicit RollingFileLogSink(
        const char* logDir,
        const char* prefix     = "server",
        int         retainDays = 7)
        : _logDir(logDir)
        , _prefix(prefix)
        , _retainDays(retainDays)
    {
        char dateBuf[16]{};
        LogFormat::FormatDate(CurrentTimestampMs(), dateBuf, sizeof(dateBuf));
        OpenFile(dateBuf);
    }

    ~RollingFileLogSink() override { CloseFile(); }

    RollingFileLogSink(const RollingFileLogSink&)            = delete;
    RollingFileLogSink& operator=(const RollingFileLogSink&) = delete;

    [[nodiscard]] bool IsOpen() const noexcept { return _file != nullptr; }

    void Write(const LogRecord& record) override
    {
        if (_file == nullptr) return;

        char dateBuf[16]{};
        LogFormat::FormatDate(record.timestampMs, dateBuf, sizeof(dateBuf));

        if (_currentDate != dateBuf)
        {
            CloseFile();
            OpenFile(dateBuf);
            if (_retainDays > 0) PurgeOldFiles();
        }

        char lineBuf[1200]{};
        LogFormat::FormatRecord(record, lineBuf, sizeof(lineBuf));
        std::fputs(lineBuf, _file);
    }

    void Flush() override
    {
        if (_file != nullptr) std::fflush(_file);
    }

private:
    std::string _logDir;
    std::string _prefix;
    int         _retainDays;
    FILE*       _file{ nullptr };
    std::string _currentDate;   // "YYYY-MM-DD"

    void OpenFile(const char* date) noexcept
    {
        _currentDate = date;

        const std::string path = _logDir + "/" + _prefix + "_" + date + ".txt";
#ifdef _WIN32
        fopen_s(&_file, path.c_str(), "a");
#else
        _file = std::fopen(path.c_str(), "a");
#endif
    }

    void CloseFile() noexcept
    {
        if (_file != nullptr)
        {
            std::fflush(_file);
            std::fclose(_file);
            _file = nullptr;
        }
    }

    void PurgeOldFiles() noexcept
    {
        try
        {
            const std::string pattern = _prefix + "_";
            std::vector<std::filesystem::path> files;

            for (const auto& entry : std::filesystem::directory_iterator(_logDir))
            {
                if (!entry.is_regular_file()) continue;
                const std::string fname = entry.path().filename().string();
                if (fname.rfind(pattern, 0) == 0 && fname.ends_with(".txt"))
                    files.push_back(entry.path());
            }

            if (static_cast<int>(files.size()) <= _retainDays) return;

            std::sort(files.begin(), files.end());
            const int toRemove = static_cast<int>(files.size()) - _retainDays;
            for (int i = 0; i < toRemove; ++i)
                std::filesystem::remove(files[i]);
        }
        catch (...) {}
    }

    static int64_t CurrentTimestampMs() noexcept
    {
        const auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
    }
};

// ============================================================
//  LevelFilteredSink  — 최소 레벨 이상만 통과시키는 래퍼
//
//  배포 환경에서 별도 에러 전용 로그 파일을 만들 때 사용:
//
//  RollingFileLogSink errorSink("Log", "error", 30);
//  LevelFilteredSink  filteredError(errorSink, LogLevel::Error);
//  FrameworkLog::Instance().AddSink(&filteredError);
// ============================================================

class LevelFilteredSink final : public ILogSink
{
public:
    LevelFilteredSink(ILogSink& inner, LogLevel minLevel)
        : _inner(inner), _minLevel(minLevel) {}

    void Write(const LogRecord& record) override
    {
        if (record.level >= _minLevel)
            _inner.Write(record);
    }

    void Flush() override { _inner.Flush(); }

private:
    ILogSink& _inner;
    LogLevel  _minLevel;
};

// ============================================================
//  LogAsyncQueue  — 더블버퍼 기반 MPSC 비동기 큐
//
//  프로듀서(로깅 스레드):
//    Push() — 짧은 락으로 front 버퍼에 push. 파일 I/O 없음.
//
//  컨슈머(백그라운드 LoggerThread):
//    Swap() — front/back 포인터 교환. 락 보유 시간 = 포인터 스왑.
//    이후 back 버퍼를 락 없이 처리하여 sink->Write() 호출.
//
//  오버플로 처리:
//    큐가 kMaxPending에 도달하면 레코드를 드롭하고 카운터 증가.
//    드롭이 복구된 시점에 "N개 드롭됨" 경고를 자동 삽입.
// ============================================================

class LogAsyncQueue
{
public:
    static constexpr size_t kMaxPending = 8192;

    // 프로듀서 스레드에서 호출 (락: 포인터 역참조 + push_back, 수 마이크로초)
    void Push(const LogRecord& record)
    {
        std::lock_guard lock{ _mtx };

        if (_front->size() >= kMaxPending)
        {
            _droppedCount.fetch_add(1, std::memory_order_relaxed);
            return;
        }

        _front->push_back(record);
        _hasData.store(true, std::memory_order_relaxed);
    }

    // 컨슈머(LoggerThread)에서 호출
    // front/back 포인터 교환 후 back을 out에 이동. 락 보유 시간 극히 짧음.
    void Swap(std::vector<LogRecord>& out)
    {
        {
            std::lock_guard lock{ _mtx };
            std::swap(_front, _back);
            _hasData.store(false, std::memory_order_relaxed);
        }

        out = std::move(*_back);
        _back->clear();
    }

    [[nodiscard]] bool HasData() const noexcept
    {
        return _hasData.load(std::memory_order_relaxed);
    }

    // 복구 시점에 삽입할 드롭 경고 레코드를 생성.
    // 드롭이 없으면 nullopt 반환.
    [[nodiscard]] bool TryMakeDropWarning(LogRecord& out) noexcept
    {
        const uint32_t dropped = _droppedCount.exchange(0, std::memory_order_relaxed);
        if (dropped == 0) return false;

        out.level        = LogLevel::Warn;
        out.category     = "Log";
        out.file         = __FILE__;
        out.line         = __LINE__;
        out.shortThreadId = 0;
        out.timestampMs  = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        std::snprintf(out.message, sizeof(out.message),
            "%u log record(s) were dropped due to queue overflow", dropped);
        return true;
    }

private:
    std::mutex _mtx;

    // 두 버퍼를 힙에 두고 포인터만 교환 — 버퍼 자체를 복사하지 않음
    std::vector<LogRecord> _bufA;
    std::vector<LogRecord> _bufB;
    std::vector<LogRecord>* _front{ &_bufA };  // 프로듀서가 쓰는 버퍼
    std::vector<LogRecord>* _back{ &_bufB };   // 컨슈머가 읽는 버퍼

    std::atomic<bool>     _hasData{ false };
    std::atomic<uint32_t> _droppedCount{ 0 };
};

// ============================================================
//  FrameworkLog  — 중앙 로거 (싱글턴)
//
//  사용 흐름 (main):
//    1. AddSink(...)            — 출력 대상 등록
//    2. SetRuntimeLevel(...)    — 런타임 필터 설정
//    3. StartWorker()           — 백그라운드 드레인 스레드 시작
//    4. [서버 동작]
//    5. StopWorker()            — 드레인 완료 후 스레드 종료
//    6. FlushAll()              — 싱크 최종 flush
//    7. ClearSinks()
//
//  로그 경로:
//    Info/Warn/Error → LogAsyncQueue → LoggerThread → sink->Write()
//    Fatal           → 동기 직접 write + FlushAll (서버 정지 전 보장)
// ============================================================

class FrameworkLog final
{
public:
    static FrameworkLog& Instance() noexcept
    {
        static FrameworkLog instance;
        return instance;
    }

    // ----- sink 관리 -----

    void AddSink(ILogSink* sink)
    {
        if (sink == nullptr) return;
        std::lock_guard lock{ _sinkMtx };
        _sinks.push_back(sink);
    }

    void RemoveSink(ILogSink* sink) noexcept
    {
        std::lock_guard lock{ _sinkMtx };
        for (auto it = _sinks.begin(); it != _sinks.end(); ++it)
        {
            if (*it == sink) { _sinks.erase(it); return; }
        }
    }

    void ClearSinks() noexcept
    {
        std::lock_guard lock{ _sinkMtx };
        _sinks.clear();
    }

    // ----- 런타임 레벨 제어 -----

    void SetRuntimeLevel(LogLevel level) noexcept
    {
        _runtimeLevel.store(static_cast<uint8_t>(level), std::memory_order_relaxed);
    }

    [[nodiscard]] LogLevel GetRuntimeLevel() const noexcept
    {
        return static_cast<LogLevel>(
            _runtimeLevel.load(std::memory_order_relaxed));
    }

    // ----- 백그라운드 드레인 스레드 -----

    // main()에서 서버 시작 전 한 번 호출
    void StartWorker()
    {
        bool expected = false;
        if (!_workerRunning.compare_exchange_strong(expected, true))
            return; // 이미 실행 중

        _workerThread = std::thread([this]{ WorkerLoop(); });
    }

    // main()에서 FlushAll() 호출 전 반드시 먼저 호출
    // 큐를 완전히 비우고 스레드를 종료한다
    void StopWorker()
    {
        if (!_workerRunning.load(std::memory_order_acquire))
            return;

        {
            std::lock_guard lock{ _cvMtx };
            _workerRunning.store(false, std::memory_order_release);
        }
        _cv.notify_all();

        if (_workerThread.joinable())
            _workerThread.join();
    }

    // ----- 로그 진입점 -----

    void Log(
        LogLevel    level,
        const char* category,
        const char* file,
        int         line,
        const char* fmt,
        ...) noexcept
    {
        if (level < GetRuntimeLevel())
            return;

        LogRecord record{};
        record.level         = level;
        record.category      = category;
        record.file          = file;
        record.line          = line;
        record.shortThreadId = GetShortThreadId();
        record.timestampMs   = GetCurrentTimestampMs();

        va_list args;
        va_start(args, fmt);
        std::vsnprintf(record.message, sizeof(record.message), fmt, args);
        va_end(args);

        // Fatal: 큐를 거치지 않고 즉시 동기 write + flush
        // 서버가 곧 종료될 상황이므로 반드시 파일에 기록되어야 함
        if (level == LogLevel::Fatal)
        {
            WriteDirect(record);
            FlushAll();
            return;
        }

        // 나머지: 비동기 큐에 push → LoggerThread가 처리
        _queue.Push(record);
        _cv.notify_one();
    }

    // 싱크 flush (StopWorker 이후에 호출)
    void FlushAll() noexcept
    {
        std::lock_guard lock{ _sinkMtx };
        for (ILogSink* sink : _sinks)
            sink->Flush();
    }

private:
    FrameworkLog()  = default;
    ~FrameworkLog() = default;

    FrameworkLog(const FrameworkLog&)            = delete;
    FrameworkLog& operator=(const FrameworkLog&) = delete;

    // ----- 싱크 상태 -----

    std::mutex             _sinkMtx;
    std::vector<ILogSink*> _sinks;
    std::atomic<uint8_t>   _runtimeLevel{ static_cast<uint8_t>(LogLevel::Trace) };

    // ----- 비동기 큐 & 백그라운드 스레드 -----

    LogAsyncQueue           _queue;
    std::thread             _workerThread;
    std::atomic<bool>       _workerRunning{ false };
    std::mutex              _cvMtx;
    std::condition_variable _cv;

    static constexpr auto kDrainInterval = std::chrono::milliseconds(10);

    void WorkerLoop()
    {
        std::vector<LogRecord> batch;
        batch.reserve(256);

        while (true)
        {
            {
                std::unique_lock lock{ _cvMtx };
                _cv.wait_for(lock, kDrainInterval, [this]
                {
                    return _queue.HasData() || !_workerRunning.load(std::memory_order_relaxed);
                });
            }

            // 드롭된 레코드가 있으면 경고를 배치 앞에 삽입
            LogRecord dropWarning{};
            if (_queue.TryMakeDropWarning(dropWarning))
                batch.push_back(dropWarning);

            _queue.Swap(batch);

            if (!batch.empty())
            {
                std::lock_guard lock{ _sinkMtx };
                for (const LogRecord& r : batch)
                    for (ILogSink* sink : _sinks)
                        sink->Write(r);
            }

            batch.clear();

            // 종료 신호가 왔고 큐도 완전히 비었으면 루프 탈출
            if (!_workerRunning.load(std::memory_order_acquire) && !_queue.HasData())
                break;
        }
    }

    // Fatal 전용 동기 직접 쓰기 (큐 우회)
    void WriteDirect(const LogRecord& record) noexcept
    {
        std::lock_guard lock{ _sinkMtx };
        for (ILogSink* sink : _sinks)
            sink->Write(record);
    }

    // ----- Thread ID 등록 테이블 -----
    // thread_local 캐시로 최초 1회만 락 획득

    std::mutex                                    _threadIdMtx;
    std::unordered_map<std::thread::id, uint32_t> _threadIdTable;
    std::atomic<uint32_t>                         _nextThreadId{ 1 };

    uint32_t GetShortThreadId() noexcept
    {
        thread_local uint32_t cachedId = 0;
        if (cachedId != 0) return cachedId;

        std::lock_guard lock{ _threadIdMtx };
        const auto tid = std::this_thread::get_id();
        const auto it  = _threadIdTable.find(tid);
        if (it != _threadIdTable.end())
        {
            cachedId = it->second;
            return cachedId;
        }

        cachedId = _nextThreadId.fetch_add(1, std::memory_order_relaxed);
        _threadIdTable[tid] = cachedId;
        return cachedId;
    }

    static int64_t GetCurrentTimestampMs() noexcept
    {
        const auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
    }
};

// ============================================================
//  편의 매크로
//
//  레벨 기준:
//    Trace — 프레임 단위 루틴, 디버그 전용
//    Debug — 초기화/종료 상세 흐름, 개발 중만
//    Info  — 서버 시작/종료, 월드 생성/소멸, 세션 연결/해제
//    Warn  — 비정상이지만 복구 가능 (재시도, 요청 거부, 드롭)
//    Error — 기능 실패, Fault 발생
//    Fatal — 서버 계속 실행 불가 (동기 write + flush 보장)
//
//  컴파일 타임 게이트: FWLOG_MIN_LEVEL 미만은 빌드 시 완전 제거.
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

#define FWLOG_TRACE(category, fmt, ...) FWLOG_IMPL(LogLevel::Trace, category, fmt, ##__VA_ARGS__)
#define FWLOG_DEBUG(category, fmt, ...) FWLOG_IMPL(LogLevel::Debug, category, fmt, ##__VA_ARGS__)
#define FWLOG_INFO(category, fmt, ...)  FWLOG_IMPL(LogLevel::Info,  category, fmt, ##__VA_ARGS__)
#define FWLOG_WARN(category, fmt, ...)  FWLOG_IMPL(LogLevel::Warn,  category, fmt, ##__VA_ARGS__)
#define FWLOG_ERROR(category, fmt, ...) FWLOG_IMPL(LogLevel::Error, category, fmt, ##__VA_ARGS__)
#define FWLOG_FATAL(category, fmt, ...) FWLOG_IMPL(LogLevel::Fatal, category, fmt, ##__VA_ARGS__)
