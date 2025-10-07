#include "Service.h"
#include "Visualizer.h"

#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")

int main()
{
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	Service::Instance().Start();
}