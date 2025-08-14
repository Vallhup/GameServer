#pragma once

enum class AiType {
	Normal,
	Boss
};

class IAiBehavior {
public:
	IAiBehavior() = delete;
	IAiBehavior(AiType type) : _type(type) {}
	virtual ~IAiBehavior() = default;

public:


protected:
	AiType _type;
};

