#include "pch.h"
#include "ServerAppTest.h"

int main()
{
	if (RunServerAppLoginSpawnFlowTest())
	{
		std::cout << "[PASS] ServerApp Smoke Test Success\n";
		return 0;
	}

	std::cout << "[FAIL] ServerApp Smoke Test Fail\n";
}