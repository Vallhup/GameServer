#include "pch.h"
#include "ServerApp.h"

#include <filesystem>
#include <string_view>

#include "FrameworkLog.h"

void RunPartySystemSmokeTests();
void RunBossGimmickCombatPolicySmokeTests();
void RunBossGimmickReplicationSmokeTests();
void RunRespawnSystemSmokeTests();

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
}

int main(int argc, char** argv)
{
	if (HasArg(argc, argv, "--party-smoke"))
	{
		RunPartySystemSmokeTests();
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

	ServerApp::Config config{};
	config.database.enabled = true;
	config.database.dsn = L"WITH_Server_DB";
	config.database.user = L"sa";
	config.database.password = L"sdong8426A";

	ServerApp app(config);
	app.Run();
	app.Shutdown();

	// 큐 완전 소진 후 스레드 종료 → 그 다음 flush
	FrameworkLog::Instance().StopWorker();
	FrameworkLog::Instance().FlushAll();
	FrameworkLog::Instance().ClearSinks();
}
