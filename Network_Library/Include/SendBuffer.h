#pragma once

struct SendBuffer {
	uint32 size;
	BYTE data[1024];

	SendBuffer* Clone() const;
};

class SendBufferPool {
public:
	static SendBufferPool& Get()
	{
		static SendBufferPool instance;
		return instance;
	}

	SendBuffer* Acquire();
	void Release(SendBuffer* buf);

private:
	concurrency::concurrent_queue<SendBuffer*> _free;
};