#pragma once

class IGameLogic {
public:
	virtual ~IGameLogic() = default;

public:
	virtual void LogicUpdate(float deltaTime) = 0;
	virtual void NetworkUpdate() = 0;
	virtual void OnPlayerAction(int sessionId, const Protocol::CS_INPUT_PACKET& packet) = 0;
};

class GameLogic : public IGameLogic {
public:
	GameLogic() = delete;
	GameLogic(Instance* instance, IEventManager* eventMng);
	virtual ~GameLogic() = default;

public:
	virtual void LogicUpdate(float deltaTime) override;
	virtual void NetworkUpdate() override;
	virtual void OnPlayerAction(int sessionId, const Protocol::CS_INPUT_PACKET& packet) override;

private:
	void ExecuteEvent(Event event);
	void HandleInput(const InputEventData& data);

private:
	Instance* _instance;
	IEventManager* _eventMng;
};