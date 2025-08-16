#pragma once

enum class AiType {
	Normal,
	Boss
};

class IAiBehavior {
public:
	IAiBehavior() = delete;
	IAiBehavior(AiType type, ScriptVM& scriptVM) : _type(type), _scriptVM(scriptVM) {}
	virtual ~IAiBehavior() = default;

public:


protected:
	AiType _type;
	ScriptVM& _scriptVM;
};