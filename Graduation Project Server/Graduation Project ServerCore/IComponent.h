#pragma once

class GameObject;
class IGameContext;
class Instance;

class IComponent {
public:
	IComponent() = delete;
	IComponent(GameObject& owner, Instance* instance);
	virtual ~IComponent() = default;

	virtual void Update(float deltaTime) {};

public:
	uint64_t Version() const { return _version.load(); }
	bool VersionCheckAndChange();

protected:
	GameObject& _owner;
	Instance* _instance;

	std::atomic<uint64_t> _version;
	uint64_t _lastSentVersion;
};