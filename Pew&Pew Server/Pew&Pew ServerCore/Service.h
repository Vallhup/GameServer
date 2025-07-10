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

	void AcceptSession();
	void CloseSession(int id);

private:
	std::shared_ptr<Listener> _listener;
	std::unordered_map<int, std::shared_ptr<Session>> _sessions;
	int _nextSessionId{ 1 };
	std::vector<int> _reusableSessionIds;
	bool _running{ false };

	int GenerateSessionId();
};

