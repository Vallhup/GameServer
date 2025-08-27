#pragma once

class TestInstance : public Instance {
public:
	TestInstance() = delete;
	TestInstance(int id, IGameContext& gameCtx) 
		: Instance(id, InstanceType::Test, gameCtx) { LoadStaticGameObject(); }
	virtual ~TestInstance() = default;

public:
	virtual void Start() override;
	virtual void Stop() override;

private:
	virtual void LoadStaticGameObject() override;
};

