#pragma once

class Listener;
class Session;

class Service : public std::enable_shared_from_this<Service>
{
public:
	Service();
	~Service();

	Service(const Service&) = delete;
	Service& operator=(const Service&) = delete;

public:
	bool Init();
	void Run();
	void Stop();

	void BroadCast(const std::vector<char>& packet, int exceptId = -1);

private:
	int GenerateSessionId();

	void AcceptSession();
	void CloseSession(int id);

private:
	std::shared_ptr<Listener> _listener;

	std::unordered_map<int, std::shared_ptr<Session>> _sessions;

	std::vector<int> _reusableSessionIds;
	bool _running{ false };
};

