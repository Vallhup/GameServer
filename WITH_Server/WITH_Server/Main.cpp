#include "pch.h"
#include "ServerApp.h"

#include "FrameworkLog.h"

static ConsoleLogSink g_consoleSink;
static FileLogSink g_fileSink("Log/WITH_Server_Log.txt");

int main()
{
	//FrameworkLog::Instance().AddSink(&g_consoleSink);
	//FrameworkLog::Instance().AddSink(&g_fileSink);

	ServerApp app;
	app.Run();
	app.Shutdown();
}