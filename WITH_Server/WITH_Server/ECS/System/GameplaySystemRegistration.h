#pragma once

class AnimationRegistry;
class WorldRuntime;

void RegisterGameplayRuntimeStorages(WorldRuntime& runtime);
void RegisterGameplayRuntimeSystems(
	WorldRuntime& runtime,
	const AnimationRegistry* animationRegistry);
