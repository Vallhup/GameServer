#include "pch.h"
#include "ServerApp.h"

#include <charconv>
#include <filesystem>
#include <limits>
#include <string>
#include <string_view>

#ifdef _WIN32
#include <Windows.h>
#endif

#include "FrameworkLog.h"

void RunPartySystemSmokeTests();
void RunCombatDamagePolicySmokeTests();
void RunBossGimmickCombatPolicySmokeTests();
void RunBossGimmickReplicationSmokeTests();
void RunRespawnSystemSmokeTests();
void RunAIBehaviorPolicySmokeTests();

namespace
{
	bool HasArg(int argc, char** argv, const char* expected)
	{
		for (int i = 1; i < argc; ++i)
		{
			if (std::string_view{ argv[i] } == expected)
				return true;
		}
		return false;
	}

	// "--key value" 형태에서 value 포인터 반환. 없으면 nullptr.
	const char* GetArgValue(int argc, char** argv, const char* key)
	{
		for (int i = 1; i + 1 < argc; ++i)
		{
			if (std::string_view{ argv[i] } == key)
				return argv[i + 1];
		}
		return nullptr;
	}

	template <typename T>
	bool TryParseUnsigned(const char* value, T& outValue)
	{
		if (value == nullptr || *value == '\0')
			return false;

		T parsed{};
		const char* const end =
			value + std::char_traits<char>::length(value);
		const auto [ptr, error] =
			std::from_chars(value, end, parsed);
		if (error != std::errc{} || ptr != end)
			return false;

		outValue = parsed;
		return true;
	}

	// UTF-8(좁은) 문자열을 std::wstring 으로 변환.
	std::wstring Widen(const char* s)
	{
		if (s == nullptr || *s == '\0')
			return std::wstring{};

#ifdef _WIN32
		const int needed =
			::MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
		if (needed <= 0)
			return std::wstring{};

		std::wstring result(static_cast<size_t>(needed - 1), L'\0');
		::MultiByteToWideChar(CP_UTF8, 0, s, -1, result.data(), needed);
		return result;
#else
		return std::wstring(s, s + std::char_traits<char>::length(s));
#endif
	}
}

int main(int argc, char** argv)
{
	if (HasArg(argc, argv, "--party-smoke"))
	{
		RunPartySystemSmokeTests();
		return 0;
	}
	if (HasArg(argc, argv, "--combat-damage-policy-smoke"))
	{
		RunCombatDamagePolicySmokeTests();
		return 0;
	}
	if (HasArg(argc, argv, "--boss-gimmick-combat-policy-smoke"))
	{
		RunBossGimmickCombatPolicySmokeTests();
		return 0;
	}
	if (HasArg(argc, argv, "--boss-gimmick-replication-smoke"))
	{
		RunBossGimmickReplicationSmokeTests();
		return 0;
	}
	if (HasArg(argc, argv, "--respawn-smoke"))
	{
		RunRespawnSystemSmokeTests();
		return 0;
	}
	if (HasArg(argc, argv, "--ai-behavior-policy-smoke"))
	{
		RunAIBehaviorPolicySmokeTests();
		return 0;
	}

	std::filesystem::create_directories("Log");

	// 전체 로그 — 날짜별 로테이션, 7일 보관
	ConsoleLogSink     consoleSink;
	RollingFileLogSink fileSink("Log", "server", 7);

	// 에러 전용 로그 — Error/Fatal만, 30일 보관 (운영 모니터링용)
	RollingFileLogSink errorFileSink("Log", "error", 30);
	LevelFilteredSink  filteredErrorSink(errorFileSink, LogLevel::Error);

	FrameworkLog::Instance().SetRuntimeLevel(LogLevel::Debug);
	FrameworkLog::Instance().AddSink(&consoleSink);
	if (fileSink.IsOpen())
	{
		FrameworkLog::Instance().AddSink(&fileSink);
	}
	if (errorFileSink.IsOpen())
	{
		FrameworkLog::Instance().AddSink(&filteredErrorSink);
	}

	// 백그라운드 드레인 스레드 시작 (파일 I/O를 메인/워커 스레드에서 분리)
	FrameworkLog::Instance().StartWorker();

	// DB 접속 정보: 기본값(fallback) 유지 → 인자로 넘기면 덮어쓴다.
	//   --db-dsn <DSN>   --db-user <USER>   --db-pass <PASSWORD>
	//   --no-db          DB 연동 비활성화
	// 예) WITH_Server.exe --db-dsn WITH_Server_DB --db-user sa --db-pass ****
	ServerApp::Config config{};
	config.database.enabled = true; //!HasArg(argc, argv, "--no-db");
	config.database.dsn = L"WITH_Server_DB";
	config.database.user = L"sa";
	config.database.password = L"sdong8426A";

	if (const char* v = GetArgValue(argc, argv, "--db-dsn"))
		config.database.dsn = Widen(v);
	if (const char* v = GetArgValue(argc, argv, "--db-user"))
		config.database.user = Widen(v);
	if (const char* v = GetArgValue(argc, argv, "--db-pass"))
		config.database.password = Widen(v);

	if (const char* v = GetArgValue(argc, argv, "--admin-account-id"))
	{
		(void)TryParseUnsigned(
			v,
			config.accountCombatStatOverride.accountId);
	}
	if (const char* v = GetArgValue(argc, argv, "--admin-max-hp"))
	{
		uint32_t value{ 0 };
		if (TryParseUnsigned(v, value) &&
			value <= static_cast<uint32_t>(
				std::numeric_limits<int32_t>::max()))
		{
			config.accountCombatStatOverride.maxHp =
				static_cast<int32_t>(value);
		}
	}
	if (const char* v = GetArgValue(argc, argv, "--admin-attack-power"))
	{
		uint32_t value{ 0 };
		if (TryParseUnsigned(v, value) &&
			value <= static_cast<uint32_t>(
				std::numeric_limits<int32_t>::max()))
		{
			config.accountCombatStatOverride.attackPower =
				static_cast<int32_t>(value);
		}
	}

	ServerApp app(config);
	app.Run();
	app.Shutdown();

	// 큐 완전 소진 후 스레드 종료 → 그 다음 flush
	FrameworkLog::Instance().StopWorker();
	FrameworkLog::Instance().FlushAll();
	FrameworkLog::Instance().ClearSinks();
}
