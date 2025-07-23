#pragma once

class Listener {
public:
	Listener() = default;
	~Listener();

public:
	bool Init();
	SOCKET Accept();

public:
	SOCKET GetSocket() const;

private:
	SOCKET _socket{ INVALID_SOCKET };
};

