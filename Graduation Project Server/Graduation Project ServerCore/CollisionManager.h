#pragma once

class CollisionManager {
public:
	void LoadFromJson(std::string_view filePath);
	std::vector<std::unique_ptr<CollisionShape>> GetShapes(std::string_view typeName) const;

private:
	std::unordered_map<std::string, std::vector<std::unique_ptr<CollisionShape>>> _shapeTable;
};

