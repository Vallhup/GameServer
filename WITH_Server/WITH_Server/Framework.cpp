#include "pch.h"
#include "Framework.h"

Framework::Framework(const Game::Config& cfg)
	: game(cfg, factory), network(4, 7000, listener), _running(false)
{
}

void Framework::Start()
{
	using namespace std::chrono;

	SetConsoleCtrlHandler(ConsoleHandler, TRUE);
	LoadAnimations();
	LoadMapDatas();

	if (!game.Init())
	{
		assert(false);
		return;
	}

	_running = true;
	network.Start();
	
	auto prev = steady_clock::now();
	while (_running)
	{
		auto now = steady_clock::now();
		double elapsed = duration<double>(now - prev).count();
		prev = now;

		game.Update(elapsed);
	}

	network.Stop();
	game.Stop();
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
	auto& aM = AnimationManager::Get();

	aM.LoadAnimation(AnimationType::Knight_Idle,		"../Animation/Knight/knight_animation_idle.json");
	aM.LoadAnimation(AnimationType::Knight_Walk,		"../Animation/Knight/knight_animation_walk.json");
	aM.LoadAnimation(AnimationType::Knight_Run,			"../Animation/Knight/knight_animation_run.json");
	aM.LoadAnimation(AnimationType::Knight_Attack,		"../Animation/Knight/knight_animation_attack.json");
	aM.LoadAnimation(AnimationType::Knight_Dead,		"../Animation/Knight/knight_animation_death.json");
	aM.LoadAnimation(AnimationType::Knight_Drinking,	"../Animation/Knight/knight_animation_drinking.json");
	aM.LoadAnimation(AnimationType::Knight_Guard,		"../Animation/Knight/knight_animation_guard.json");
	aM.LoadAnimation(AnimationType::Knight_Hit,			"../Animation/Knight/knight_animation_hit.json");
	aM.LoadAnimation(AnimationType::Knight_Parry,		"../Animation/Knight/knight_animation_parry.json");
	aM.LoadAnimation(AnimationType::Knight_Dodge,		"../Animation/Knight/knight_animation_dodge.json");
	aM.LoadAnimation(AnimationType::Knight_Stun,		"../Animation/Knight/knight_animation_stun.json");
	
	

	aM.LoadAnimation(AnimationType::FinalBoss_Idle,			"../Animation/Final_Boss/final_boss_animation_idle.json");
	aM.LoadAnimation(AnimationType::FinalBoss_Walk,			"../Animation/Final_Boss/final_boss_animation_walk.json");
	aM.LoadAnimation(AnimationType::FinalBoss_Thrust,		"../Animation/Final_Boss/final_boss_animation_thrust.json");
	aM.LoadAnimation(AnimationType::FinalBoss_Slash,		"../Animation/Final_Boss/final_boss_animation_slash.json");
	aM.LoadAnimation(AnimationType::FinalBoss_DashSlash,	"../Animation/Final_Boss/final_boss_animation_dashslash.json");
	aM.LoadAnimation(AnimationType::FinalBoss_JumpSlash,	"../Animation/Final_Boss/final_boss_animation_jumpslash.json");
	aM.LoadAnimation(AnimationType::FinalBoss_MultiSlash,	"../Animation/Final_Boss/final_boss_animation_multislash.json");
	aM.LoadAnimation(AnimationType::FinalBoss_Dead,			"../Animation/Final_Boss/final_boss_animation_death.json");
	aM.LoadAnimation(AnimationType::FinalBoss_Hit,			"../Animation/Final_Boss/final_boss_animation_hit.json");
	aM.LoadAnimation(AnimationType::FinalBoss_Stun,			"../Animation/Final_Boss/final_boss_animation_stun.json");

	aM.LoadActionAnimationMap();
}

void Framework::LoadMapDatas()
{
	MapCollisionManager::Get().LoadMapData("../Map/map_1_collision.png");

	MapCollisionManager::Get().LoadHeightMap("../Map/map1_terrain.raw");
}
	