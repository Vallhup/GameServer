#include "pch.h"
#include <iostream>

#include "Benchmark.h"

void ServerStart()
{
	setlocale(LC_ALL, "korean");

	Logger::Init("", "C:/Users/Hadenpel/Desktop/GameServer/Graduation Project Server/Graduation Project ServerCore/");
	Logger::SetLevel(LogLevel::Error);

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
	//ServerStart();
	CollisionManager mgr;
	mgr.LoadFromJson("animation/knight_cylinders.json");

	const auto shapes = mgr.GetShapes("Knight");
	if(shapes.empty()) 	{
		std::cout << "No shapes found for 'Knight'\n";
		return 0;
	}

	for (auto& shape : shapes) {
		const auto* c = static_cast<CylinderShape*>(shape.get());
		std::cout << "Shape: offset(" 
			<< c->GetLocalOffset().x << ", " << c->GetLocalOffset().y << ", " << c->GetLocalOffset().z
			<< "), radius: " << c->GetRadius() << ", height: " << c->GetHeight() << ", direction(" 
			<< c->GetDirection().x << ", " << c->GetDirection().y << ", " << c->GetDirection().z << ")\n";
	}
}