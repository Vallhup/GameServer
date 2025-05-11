#pragma once

#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

enum LogLevel : char
{
	Debug,
	Info,
	Warn,
	Error
};

class SpinLock {
public:
	void lock() {
		while (_flag.test_and_set(std::memory_order_acquire)) { /* spin */ }
	}
	void unlock() {
		_flag.clear(std::memory_order_release);
	}

private:
	std::atomic_flag _flag = ATOMIC_FLAG_INIT;
};


class Logger
{
public:
	static void Init(const std::string& filename = "", const std::string& basePath = "",
		LogLevel level = LogLevel::Info)
	{
		if (not filename.empty()) {
			_ofs = std::make_unique<std::ofstream>(filename, std::ios::app);
		}
		_basePath = NormalizePath(basePath);
		_level.store(level, std::memory_order_relaxed);
	}

	static void SetLevel(LogLevel level) { _level.store(level, std::memory_order_relaxed); }

	static void Log(LogLevel level, const char* file, int line, const char* fmt, ...)
	{
		if (level < _level.load(std::memory_order_relaxed))
			return;

		auto now = std::chrono::system_clock::now();
		auto in_time_t = std::chrono::system_clock::to_time_t(now);

		std::tm bt;
		localtime_s(&bt, &in_time_t);

		std::ostringstream ts;
		ts << std::put_time(&bt, "%Y-%m-%d %H:%M:%S");

		const char* levelString = "";
		switch (level) {
		case LogLevel::Debug: levelString = "DBG"; break;
		case LogLevel::Info: levelString = "INF"; break;
		case LogLevel::Warn: levelString = "WRN"; break;
		case LogLevel::Error: levelString = "ERR"; break;
		}

		std::string path = NormalizePath(file);

		if (!_basePath.empty() && path.rfind(_basePath, 0) == 0) {
			path.erase(0, _basePath.size());
		}

		// 별도로 Log를 출력할 파일이 정해져있으면 해당 파일로, 없으면 콘솔에 출력
		std::ostream& os = _ofs ? *_ofs : std::cout;
		{
			_spin.lock();

			os << "[" << ts.str() << "][" << levelString << "][" << path << ":" << line << "] ";

			va_list ap;
			va_start(ap, fmt);
			char msg[512];
			std::vsnprintf(msg, sizeof(msg), fmt, ap);
			va_end(ap);

			os << msg << "\n";
			os.flush();

			_spin.unlock();
		}
	}

private:
	static std::string NormalizePath(const std::string& p)
	{
		std::string s = p;
		std::replace(s.begin(), s.end(), '\\', '/');
		if (!s.empty() && s.back() != '/') s += '/';
		return s;
	}

private:
	static inline SpinLock							_spin;
	static inline std::string						_basePath;
	static inline std::atomic<LogLevel>				_level{ LogLevel::Info };
	static inline std::unique_ptr<std::ofstream>	_ofs;
	
};

