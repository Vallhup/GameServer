#include "pch.h"
#include <iostream>

#include "Benchmark.h"

void ServerStart()
{
	setlocale(LC_ALL, "korean");

	Logger::Init("", "C:/Users/Hadenpel/Desktop/GameServer/Graduation Project Server/Graduation Project ServerCore/");
	Logger::SetLevel(LogLevel::Debug);

	auto service = std::make_unique<Service>();

	service->Init();
	service->Start();

	std::cout << "종료하려면 Enter 키를 누르세요...\n";
	std::cin.get();

	service->Stop();

	Logger::Shutdown();
}

int main()
{
	//ObjectPoolBenchmark();
	ServerStart();
}