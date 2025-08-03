#pragma once

#define WIN_LEAN_AND_WEAN

#ifdef _DEBUG
#pragma comment(lib, "Debug\\Graduation Project ServerCore.lib")
#else
#pragma comment(lib, "Release\\Graduation Project ServerCore.lib")
#endif

#include "CorePch.h"