#include "Service.h"

int main()
{
	setlocale(LC_ALL, "korean");

	auto service = std::make_unique<Service>();

	service->Start();

	std::cout << "종료하려면 Enter 키를 누르세요...\n";
	std::cin.get();

	service->Stop();
}