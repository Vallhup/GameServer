#pragma once

class PvpInstance : public Instance {
public:
	PvpInstance() = delete;
	PvpInstance(int id, IGameContext& gameCtx) : Instance(id, gameCtx) { LoadStaticGameObject(); }
	virtual ~PvpInstance() = default;

public:
	virtual void Update(float deltaTime) override;
	virtual void Stop() override;

private:
	virtual void LoadStaticGameObject() override;
};
