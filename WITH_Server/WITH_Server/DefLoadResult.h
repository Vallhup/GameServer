#pragma once

#include <cstddef>
#include <string>

struct DefLoadResult
{
	bool succeeded{ false };
	size_t loadedCount{ 0 };
	std::string error;
};
