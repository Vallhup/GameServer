#include "pch.h"
#include "ServerApp.h"

#include <filesystem>

#include "FrameworkLog.h"

int main()
{
	std::filesystem::create_directories("Log");

	//ConsoleLogSink consoleSink;
	FileLogSink fileSink("Log/WITH_Server_Log.txt");

	FrameworkLog::Instance().SetRuntimeLevel(LogLevel::Info);
	//FrameworkLog::Instance().AddSink(&consoleSink);
	if (fileSink.IsOpen())
	{
		FrameworkLog::Instance().AddSink(&fileSink);
	}

	ServerApp app;
	app.Run();
	app.Shutdown();

	FrameworkLog::Instance().FlushAll();
	FrameworkLog::Instance().ClearSinks();
}
