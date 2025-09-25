#pragma once

#include <sqlext.h>

// 추가적으로 Service등 상위 Class에서 DB Init 해주는 코드 필요
//
// 1. ODBC Environment Handle Alloc
// 2. ODBC Handle Alloc
// 3. SQLConnect

struct UserData {
	// TEMP : DB Load or Save할 때 사용할 구조체
	//        DB Table에 따라 늘어날 예정
};

class IUserRepository {
public:
	virtual ~IUserRepository() = default;

public:
	// 임시로 userId int로 해놨는데 추후 string or wstring으로 바꿀 예정
	virtual bool LoadUser(int userId, UserData& outData) = 0;
	virtual bool SaveUser(const UserData& userData) = 0;
	virtual bool UpdateUser(int userId, UserData& userData) = 0;
	virtual bool DeleteUser(int userId, UserData& userData) = 0;
};

// 어떤 DB 사용할 지 고민중 (아마 MsSQL 쓸 듯)
class MsSQLUserRepository : public IUserRepository {
public:
	MsSQLUserRepository() = default;
	virtual ~MsSQLUserRepository() = default;

public:
	virtual bool LoadUser(int userId, UserData& outData) override;
	virtual bool SaveUser(const UserData& userData) override;
	virtual bool UpdateUser(int userId, UserData& userData) override;
	virtual bool DeleteUser(int userId, UserData& userData) override;

public:
	// 비동기 DB 처리

	// 1. Callback Function
	// 2. Event Queue
	// 3. Future / Promise

	// 보통 1, 2번 방법 합쳐서 많이 사용하고
	// 3번 같은 방식은 C# 서버에서 많이 사용하는데
	// 코드 가독성 측면에서 3번 사용해볼까 고민중

	std::future<UserData> LoadUserAsync(int userId);
	std::future<bool> SaveUserAsync(const UserData& userData);
	std::future<bool> UpdateUserAsync(const UserData& userData);
	std::future<bool> DeleteUserAsync(int userId);
};