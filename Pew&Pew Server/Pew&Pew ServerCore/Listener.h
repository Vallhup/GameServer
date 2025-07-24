#pragma once

class Listener {
public:
	Listener() = default;
	~Listener();

	Listener(const Listener&) = delete;
	Listener& operator=(const Listener&) = delete;

	Listener(Listener&&) = delete;
	Listener& operator=(Listener&&) = delete;

public:
	bool Init(short portNum = 9000);
	void Stop();
	SOCKET Accept();

public:
	SOCKET GetSocket() const;

private:
	SOCKET _socket{ INVALID_SOCKET };
};

