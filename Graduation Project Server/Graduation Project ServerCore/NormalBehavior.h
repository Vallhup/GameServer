#pragma once

class NormalBehavior : public IAiBehavior {
public:
	NormalBehavior() = delete;
	NormalBehavior(ScriptVM& scriptVM) : IAiBehavior(AiType::Normal, scriptVM) {}
	virtual ~NormalBehavior() = default;

public:

private:

};

