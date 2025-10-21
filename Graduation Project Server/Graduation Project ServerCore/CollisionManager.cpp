#include "pch.h"
#include "CollisionManager.h"

#include "json.hpp"

void CollisionManager::LoadFromJson(std::string_view filePath)
{
	std::ifstream ifs(filePath.data());
	if (not ifs.is_open()) {
		LOG_ERR("Failed to open collision shape file");
		return;
	}

	nlohmann::json jsonData;
	ifs >> jsonData;

	std::string objectName = jsonData["ObjectName"];

	auto meshes = jsonData["Meshes"];
	for (auto& [meshName, bones] : meshes.items()) {
		for (auto& [boneId, shapeData] : bones.items()) {
			vec3 offset{
				shapeData["Offset"][0],
				shapeData["Offset"][1],
				shapeData["Offset"][2]
			};

			vec3 direction{
				shapeData["Direction"][0],
				shapeData["Direction"][1],
				shapeData["Direction"][2]
			};

			float radius = shapeData["Radius"];
			float height = shapeData["Height"];

			auto shape = std::make_unique<CylinderShape>(
				CollisionType::Hurt,
				offset, radius, height, direction
			);
			_shapeTable[objectName].push_back(std::move(shape));
		}
	}

	LOG_DBG("Loaded collision shapes from file: {}", filePath);
}

std::vector<std::unique_ptr<CollisionShape>> CollisionManager::GetShapes(std::string_view typeName) const
{
	std::vector<std::unique_ptr<CollisionShape>> shapes;
	auto it = _shapeTable.find(typeName.data());
	if(it != _shapeTable.end()) {
		for (const auto& shape : it->second) {
			shapes.push_back(shape->Clone());
		}
	}

	return shapes;
}
