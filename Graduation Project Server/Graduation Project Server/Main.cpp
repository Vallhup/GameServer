#include "pch.h"
#include <iostream>

int main()
{
	setlocale(LC_ALL, "korean");

	Logger::Init();
	Logger::SetLevel(LogLevel::Error);

	auto service = std::make_unique<Service>();

	service->Init();
	service->Start();

	std::cout << "종료하려면 Enter 키를 누르세요...\n";
	std::cin.get();

	service->Stop();

	Logger::Shutdown();
}