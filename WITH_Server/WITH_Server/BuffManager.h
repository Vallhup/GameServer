#pragma once

#include <array>
#include "BuffData.h"

class BuffManager {
	using BuffTable = std::array<BuffDef, buffCnt>;

public:
	static BuffManager& Get()
	{
		static BuffManager instance;
		return instance;
	}

	void LoadBuffDatas(std::string_view path);
	const BuffDef& GetBuff(BuffType type) const;

private:
	BuffManager() = default;

	void SetBuff(const BuffDef& buff);

	BuffTable _buffTable;
};

