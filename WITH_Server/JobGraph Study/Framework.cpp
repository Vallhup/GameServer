#include "Framework.h"

Framework::Framework(size_t size)
	: game(size), network(7000, size), _running(false)
{
	AnimationManager::Get().LoadAnimation(AnimationId::Knight_Idle, "C:/Users/Hadenpel/Knight_Walk_COLLIDER_PREBAKED.json");
	AnimationManager::Get().LoadAnimation(AnimationId::Knight_Walk, "C:/Users/Hadenpel/Knight_Walk_COLLIDER_PREBAKED.json");
	//AnimationManager::Get().LoadAnimation(AnimationId::Knight_Run, "C:/Users/Hadenpel/Knight_Walk_COLLIDER_PREBAKED.json");
}

void Framework::Start()
{
	using namespace std::chrono;

	SetConsoleCtrlHandler(ConsoleHandler, TRUE);

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