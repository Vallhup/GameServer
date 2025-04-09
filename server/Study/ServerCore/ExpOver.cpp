#include "pch.h"
#include "ExpOver.h"

ExpOver::ExpOver(OperationType operationType) : _operationType(operationType), _owner(nullptr)
{
	Init();
}

void ExpOver::Init()
{
	std::memset(this, 0, sizeof(OVERLAPPED));
}