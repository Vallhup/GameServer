#pragma once


class TownInstance : public Instance {
public:
	TownInstance() = delete;
	TownInstance(int id, IGameContext& gameCtx) : Instance(id, InstanceType::Town, gameCtx) { LoadStaticGameObject(); }
	virtual ~TownInstance() = default;

public:
	virtual void Start() override;
	virtual void Stop() override;

private:
	virtual void LoadStaticGameObject() override;
};
