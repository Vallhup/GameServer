#include "pch.h"
#include "ServerApp.h"

#include <filesystem>

#include "FrameworkLog.h"

int main()
{
	//std::filesystem::create_directories("Log");

	//// 전체 로그 — 날짜별 로테이션, 7일 보관
	//ConsoleLogSink     consoleSink;
	//RollingFileLogSink fileSink("Log", "server", 7);

	//// 에러 전용 로그 — Error/Fatal만, 30일 보관 (운영 모니터링용)
	//RollingFileLogSink errorFileSink("Log", "error", 30);
	//LevelFilteredSink  filteredErrorSink(errorFileSink, LogLevel::Error);

	//FrameworkLog::Instance().SetRuntimeLevel(LogLevel::Info);
	//FrameworkLog::Instance().AddSink(&consoleSink);
	//if (fileSink.IsOpen())
	//{
	//	FrameworkLog::Instance().AddSink(&fileSink);
	//}
	//if (errorFileSink.IsOpen())
	//{
	//	FrameworkLog::Instance().AddSink(&filteredErrorSink);
	//}

	//// 백그라운드 드레인 스레드 시작 (파일 I/O를 메인/워커 스레드에서 분리)
	//FrameworkLog::Instance().StartWorker();

	ServerApp::Config config{};
	config.database.enabled = false;
	config.database.connectionString =
		L"Driver={ODBC Driver 17 for SQL Server};"
		L"Server=localhost\\SQLEXPRESS;"
		L"Database=WITH_Server_DB;"
		L"Trusted_Connection=yes;"
		L"Encrypt=yes;"
		L"TrustServerCertificate=yes;";

	ServerApp app(config);
	app.Run();
	app.Shutdown();

	//// 큐 완전 소진 후 스레드 종료 → 그 다음 flush
	//FrameworkLog::Instance().StopWorker();
	//FrameworkLog::Instance().FlushAll();
	//FrameworkLog::Instance().ClearSinks();
}
