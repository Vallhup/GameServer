#include "pch.h"
#include "AIComponent.h"

AIComponent::AIComponent(GameObject& owner, Instance& instance, AiType type, ScriptVM& scriptVM) : IComponent(owner, instance) 
{
	// AiType에 맞는 Behavior 생성
}