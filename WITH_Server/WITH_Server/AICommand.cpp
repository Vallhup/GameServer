#include "pch.h"
#include "AICommand.h"

bool IsAIMoveCommandType(WorldCommandTypeKey typeKey) noexcept
{
	return typeKey == static_cast<WorldCommandTypeKey>(AICommandTypeKey::Move);
}

bool IsAIActionCommandType(WorldCommandTypeKey typeKey) noexcept
{
	return typeKey == static_cast<WorldCommandTypeKey>(AICommandTypeKey::Action);
}
