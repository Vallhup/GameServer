#pragma once

class MainInstance : public Instance {
public:
	MainInstance() = delete;
	MainInstance(int id, IGameContext& gameCtx) : Instance(id, InstanceType::Main, gameCtx) { LoadStaticGameObject(); }
	virtual ~MainInstance() = default;

public:
	virtual void Start() override;
	virtual void Stop() override;

private:
	virtual void LoadStaticGameObject() override;
};
