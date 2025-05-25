#pragma once

#define _HAS_STD_BYTE 0

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN  
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <iostream>
#include <sstream>
#include <string>
#include <memory>
#include <bitset>
#include <array>
#include <vector>
#include <utility>
using namespace std;

#include "d3dx12.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>

using namespace DirectX;
using namespace Microsoft::WRL;

#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3dcompiler.lib")

#ifdef _DEBUG
#pragma comment(lib, "libfbxsdk-md.lib")
#pragma comment(lib, "libxml2-md.lib")
#pragma comment(lib, "zlib-md.lib")
#else
#pragma comment(lib, "libfbxsdk-md.lib")
#pragma comment(lib, "libxml2-md.lib")
#pragma comment(lib, "zlib-md.lib")
#endif

#if !defined(ASSERT)
#include <cassert>
#define ASSERT(expr) assert(expr)
#endif

#if !defined(MASSERT)
#define MASSERT(expr, msg) \
	if (!(expr)) { \
		OutputDebugStringA("Assertion Failed: "); \
		OutputDebugStringA(msg); \
		OutputDebugStringA("\n"); \
		__debugbreak(); \
	}
#endif				

#define GET(type) type::Get()

#if !defined(RELEASE_COM)
#define RELEASE_COM(x) \
	if ((x) != nullptr) \
	{ \
		(x)->Release(); \
		(x) = nullptr; \
	} \
	((void)0)
#endif

using UINT32 = unsigned __int32;
using UINT64 = unsigned __int64;

constexpr XMINT2 WinSize(800, 600);
constexpr int SWAP_CHAIN_BUFFER_COUNT = 2;

inline static float GetAngleBetweenNormals(const XMVECTOR& v0, const XMVECTOR& v1)
{
	return XMConvertToDegrees(XMVectorGetX(XMVector3AngleBetweenNormals(v0, v1)));
}

inline static float GetAngleBetweenVectors(const XMVECTOR& v0, const XMVECTOR& v1)
{
	return GetAngleBetweenNormals(XMVector3Normalize(v0), XMVector3Normalize(v1));
}