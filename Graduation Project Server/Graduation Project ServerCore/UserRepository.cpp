#include "pch.h"
#include "UserRepository.h"

std::future<UserData> MsSQLUserRepository::LoadUserAsync(int userId)
{
	// TODO : ODBC Handle Alloc, SQLConnect

    // IOCP Worker Thread로 작업 넘겨서 비동기로 실행 후 future객체로 return
    // 하는 법 몰라서 공부해야 됨
    {
        // 1. Statement Handle Alloc
        // 2. Query 실행
        // 3. Column Binding
    }
}
