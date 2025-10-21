#pragma once

class GameObject;
class IGameContext;
class Instance;

class IComponent {
public:
	IComponent() = delete;
	IComponent(GameObject& owner, Instance* instance);
	virtual ~IComponent() = default;

	virtual void LogicUpdate(float deltaTime) {};
	virtual void NetworkUpdate() {};

public:
	uint64_t Version() const { return _version; }
	bool VersionCheckAndChange();

protected:
	GameObject& _owner;
	Instance* _instance;

	uint64_t _version;
	uint64_t _lastSentVersion;
};