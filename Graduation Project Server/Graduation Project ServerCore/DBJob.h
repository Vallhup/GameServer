#pragma once

class DBJob : public Job {
public:
	DBJob() = delete;
	DBJob(IGameContext& gameCtx) : _gameCtx(gameCtx) {}
	virtual ~DBJob() = default;

protected:
	IGameContext& _gameCtx;
};

class LoginJob : public DBJob {
public:
	LoginJob() = delete;
	LoginJob(IGameContext& gameCtx, std::wstring_view id, std::wstring_view password, Session* session);
	virtual ~LoginJob() = default;

public:
	virtual void Execute() override;

private:
	std::wstring _id;
	std::wstring _password;
	Session* _session;
};
