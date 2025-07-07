#include "pch.h"
#include "Engine.h"

unique_ptr<Engine> GEngine = make_unique<Engine>();

int main(int argc, char* argv[])
{
	GEngine->Init();
	GEngine->Update();
	GEngine->Release();
}