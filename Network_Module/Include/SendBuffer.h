#pragma once

struct SendBuffer {
	uint32 size;
	BYTE data[1024];

	SendBuffer* Clone() const;
};

class SendBufferPool {
public:
	SendBuffer* Acquire();
	void Release(SendBuffer* buf);

private:
	std::vector<SendBuffer*> _free;
};