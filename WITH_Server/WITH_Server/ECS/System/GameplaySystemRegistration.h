#pragma once

class AnimationRegistry;
class WorldRuntime;

// Note: 컴포넌트 storage 등록은 CharacterAspectRegistry::RegisterStoragesAll 로 이관됨.
// 이 함수는 시스템 등록만 담당한다.
void RegisterGameplayRuntimeSystems(
	WorldRuntime& runtime,
	const AnimationRegistry* animationRegistry);
