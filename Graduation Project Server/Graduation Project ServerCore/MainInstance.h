#pragma once

class MainInstance : public Instance {
public:
	MainInstance() = delete;
	MainInstance(int id, IGameContext& gameCtx) : Instance(id, gameCtx) { LoadStaticGameObject(); }
	virtual ~MainInstance() = default;

public:
	virtual void Update(float deltaTime) override;
	virtual void Stop() override;

private:
	virtual void LoadStaticGameObject() override;
};
