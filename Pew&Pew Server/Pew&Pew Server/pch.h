#pragma once

#define WIN_LEAN_AND_WEAN

#ifdef _DEBUG
#pragma comment(lib, "Debug\\Pew&Pew ServerCore.lib")
#else
#pragma comment(lib, "Release\\Pew&Pew ServerCore.lib")
#endif

#include "CorePch.h"