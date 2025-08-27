#pragma once

class PvpInstance : public Instance {
public:
	PvpInstance() = delete;
	PvpInstance(int id, IGameContext& gameCtx) : Instance(id, InstanceType::Pvp, gameCtx) { LoadStaticGameObject(); }
	virtual ~PvpInstance() = default;

public:
	virtual void Start() override;
	virtual void Stop() override;

private:
	virtual void LoadStaticGameObject() override;
};
