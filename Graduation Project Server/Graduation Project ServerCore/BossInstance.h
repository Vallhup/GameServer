#pragma once

class BossInstance : public Instance {
public:
	BossInstance() = delete;
	BossInstance(int id, IGameContext& gameCtx) : Instance(id, InstanceType::Boss, gameCtx) { LoadStaticGameObject(); }
	virtual ~BossInstance() = default;

public:
	virtual void Start() override;
	virtual void Stop() override;

private:
	virtual void LoadStaticGameObject() override;
};
