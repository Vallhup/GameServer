#pragma once

class GameObject;
class IGameContext;

class IComponent {
public:
	IComponent() = delete;
	IComponent(GameObject& owner, IGameContext& gameCtx) 
		: _owner(owner), _gameCtx(gameCtx) { _version = 0; }
	virtual ~IComponent() = default;

public:
	virtual void Update(float deltaTime) = 0;

public:
	uint64_t Version() const { return _version; }

protected:
	GameObject& _owner;
	IGameContext& _gameCtx;

	uint64_t _version;
};