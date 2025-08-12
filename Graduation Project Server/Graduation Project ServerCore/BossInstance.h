#pragma once

class BossInstance : public Instance {
public:
	BossInstance() = delete;
	BossInstance(int id, IGameContext& gameCtx) : Instance(id, gameCtx) { LoadStaticGameObject(); }
	virtual ~BossInstance() = default;

public:
	virtual void Update(float deltaTime) override;
	virtual void Stop() override;

private:
	virtual void LoadStaticGameObject() override;
};
