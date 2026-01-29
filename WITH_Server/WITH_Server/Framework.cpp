#include "pch.h"
#include "Framework.h"

Framework::Framework(size_t size)
	: game(4), network(8, 7000, listener), _running(false)
{
}

void Framework::Start()
{
	using namespace std::chrono;

	SetConsoleCtrlHandler(ConsoleHandler, TRUE);
	LoadAnimations();
	LoadMapColliders();

	_running = true;
	network.Start();
	
	const float dT = 1.0f / 60.0f;
	auto prev = steady_clock::now();
	while (_running)
	{
		auto now = steady_clock::now();
		float elapsed = duration<float>(now - prev).count();

		if (elapsed >= dT)
		{
			prev = now;
			game.Update(dT);
		}
	}

	network.Stop();
	game.threadPool.Stop();
}

void Framework::Stop()
{
	_running = false;
}

BOOL __stdcall Framework::ConsoleHandler(DWORD ctrlType)
{
	switch (ctrlType) {
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		Framework::Get().Stop();
		return TRUE;
	}

	return FALSE;
}

void Framework::LoadAnimations()
{
	AnimationManager::Get().LoadAnimation(AnimationType::Knight_Idle, "../Animation/Knight/knight_animation_idle.json");
	AnimationManager::Get().LoadAnimation(AnimationType::Knight_Walk, "../Animation/Knight/knight_animation_walk.json");
	AnimationManager::Get().LoadAnimation(AnimationType::Knight_Run, "../Animation/Knight/knight_animation_run.json");
	AnimationManager::Get().LoadAnimation(AnimationType::Knight_Attack, "../Animation/Knight/knight_animation_attack.json");
	AnimationManager::Get().LoadAnimation(AnimationType::Knight_Dead, "../Animation/Knight/knight_animation_death.json");
	AnimationManager::Get().LoadAnimation(AnimationType::Knight_Drinking, "../Animation/Knight/knight_animation_drinking.json");
	AnimationManager::Get().LoadAnimation(AnimationType::Knight_Guard, "../Animation/Knight/knight_animation_guard.json");
	AnimationManager::Get().LoadAnimation(AnimationType::Knight_Hit, "../Animation/Knight/knight_animation_hit.json");
	AnimationManager::Get().LoadAnimation(AnimationType::Knight_Parry, "../Animation/Knight/knight_animation_parry.json");
	AnimationManager::Get().LoadAnimation(AnimationType::Knight_Dodge, "../Animation/Knight/knight_animation_dodge.json");
	AnimationManager::Get().LoadAnimation(AnimationType::Knight_Stun, "../Animation/Knight/knight_animation_stun.json");

	AnimationManager::Get().LoadActionAnimationMap();
}

void Framework::LoadMapColliders()
{
	MapCollisionManager::Get().
		LoadCharacterCollider(CharacterType::Knight, "../Animation/Knight/knight_map_capsules.json");
}
