#include "pch.h"
#include "ExpOver.h"

ExpOver::ExpOver(OperationType operationType) : _operationType(operationType), _owner(nullptr)
{
	Init();
}

void ExpOver::Init()
{
	OVERLAPPED::hEvent = 0;
	OVERLAPPED::Internal = 0;
	OVERLAPPED::InternalHigh = 0;
	OVERLAPPED::Offset = 0;
	OVERLAPPED::OffsetHigh = 0;
}