#include "pch.h"
#include "MapCollisionManager.h"

CharacterMapCapsule* MapCollisionManager::GetCharacterCollider(CharacterType type)
{
    return _charMapCapsules[ToInt(type)].get();
}

void MapCollisionManager::LoadMapCollider(std::string_view path)
{
}

void MapCollisionManager::LoadCharacterCollider(CharacterType type, std::string_view path)
{
	auto col = std::make_unique<CharacterMapCapsule>(LoadCharacterColliderInternal(path));

#ifdef _DEBUG
	if (col.get() != nullptr)
		std::cout << "MapCollider Load Success: " << path << std::endl;
#endif

	_charMapCapsules[ToInt(type)] = std::move(col);
}

CharacterMapCapsule MapCollisionManager::LoadCharacterColliderInternal(std::string_view path)
{
	CharacterMapCapsule out;
	std::vector<CapsuleView> capsules;

	std::ifstream ifs(path.data());
	if (!ifs.is_open())
		throw std::runtime_error("파일을 열 수 없습니다: " + std::string(path));

	json j;
	ifs >> j;

    const json& meshes = j.at("Meshes"); 

    for (const auto& [meshName, meshObj] : meshes.items())
    {
        for (const auto& [idx, m] : meshObj.items())
        {
            const float radius = m.at("radius").get<float>();
            const float halfHeight = m.at("halfHeight").get<float>();

            const auto& c = m.at("center");
            const XMFLOAT3 center
            {
                c.at(0).get<float>(),
                c.at(1).get<float>(),
                c.at(2).get<float>()
            };

            const auto& d = m.at("direction");
            const XMFLOAT3 dir
            {
                d.at(0).get<float>(),
                d.at(1).get<float>(),
                d.at(2).get<float>()
            };

            capsules.push_back(MakeCapsuleView(center, dir, halfHeight, radius));
        }
    }

	if (capsules.size() == 2)
	{
		out.stand = capsules[0];
		out.dodge = capsules[1];

		return out;
	}

	throw std::runtime_error("파일 포맷 이상:" + std::string(path));
}
