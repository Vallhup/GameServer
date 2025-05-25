#pragma once
#include <string>
#include <vector>

struct Vertex;

bool ExportToBinary(const std::wstring& path, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
bool ExportToText(const std::wstring& path, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);