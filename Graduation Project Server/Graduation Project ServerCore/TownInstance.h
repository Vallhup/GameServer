#pragma once


class TownInstance : public Instance {
public:
	TownInstance() = delete;
	TownInstance(int id, IGameContext& gameCtx) : Instance(id, gameCtx) { LoadStaticGameObject(); }
	virtual ~TownInstance() = default;

public:
	virtual void Update(float deltaTime) override;
	virtual void Stop() override;

private:
	virtual void LoadStaticGameObject() override;
};
