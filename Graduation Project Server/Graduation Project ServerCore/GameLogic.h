#pragma once

class IGameLogic {
public:
	virtual ~IGameLogic() = default;

public:
	virtual void LogicUpdate(float deltaTime) = 0;
	virtual void NetworkUpdate() = 0;
	virtual void OnPlayerAction(int sessionId, const std::vector<char>& packet) = 0;
};

class GameLogic : public IGameLogic {
	using PacketHandler = std::function<void(int, const std::vector<char>&)>;

public:
	GameLogic() = delete;
	GameLogic(IGameContext& gameCtx);
	virtual ~GameLogic() = default;

public:
	virtual void LogicUpdate(float deltaTime) override;
	virtual void NetworkUpdate() override;
	virtual void OnPlayerAction(int sessionId, const std::vector<char>& packet) override;

private:
	void RegisterHandlers();

private:
	IGameContext& _gameCtx;

	std::unordered_map<unsigned char, PacketHandler> _packetHandlers;
};

