#pragma once

#include <Windows.h>
#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <fstream>
using namespace std;

#include <filesystem>
namespace fs = std::filesystem;

#include "d3dx12.h"
#include "SimpleMath.h"
#include <d3d12.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include "fbxsdk.h"
using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;

#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "d3dcompiler")

#ifdef _DEBUG
#pragma comment(lib, "../Library/Lib/FBX/debug/libfbxsdk-md.lib")
#pragma comment(lib, "../Library/Lib/FBX/debug/libxml2-md.lib")
#pragma comment(lib, "../Library/Lib/FBX/debug/zlib-md.lib")
#else
#pragma comment(lib, "../Library/Lib/FBX/release/libfbxsdk-md.lib")
#pragma comment(lib, "../Library/Lib/FBX/release/libxml2-md.lib")
#pragma comment(lib, "../Library/Lib/FBX/release/zlib-md.lib")
#endif

using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;
using Vec2 = DirectX::SimpleMath::Vector2;
using Vec3 = DirectX::SimpleMath::Vector3;
using Vec4 = DirectX::SimpleMath::Vector4;
using Matrix = DirectX::SimpleMath::Matrix;

struct Vertex
{
	Vertex() {}

	Vertex(Vec3 p, Vec2 u, Vec3 n, Vec3 t)
		: pos(p), uv(u), normal(n), tangent(t)
	{
	}

	Vec3 pos;
	Vec2 uv;
	Vec3 normal;
	Vec3 tangent;
	Vec4 weights;
	Vec4 indices;
	Vec4 color;
};

wstring s2ws(const string& s);
string ws2s(const wstring& s);