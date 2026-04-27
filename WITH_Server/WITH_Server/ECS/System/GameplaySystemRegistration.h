#pragma once

class AnimationRegistry;
class SystemManager;
class WorldRuntime;

class GameplaySystemRegistrar final
{
public:
	explicit GameplaySystemRegistrar(
		const AnimationRegistry* animationRegistry) noexcept;

	void Register(WorldRuntime& runtime) const;
	void Register(SystemManager& systemManager) const;

private:
	template<typename TargetT>
	void RegisterSystems(TargetT& target) const;

private:
	const AnimationRegistry* _animationRegistry{ nullptr };
};
