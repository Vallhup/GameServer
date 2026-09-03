#pragma once

#define _HAS_STD_BYTE 0

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN  
#include <winsock2.h>    
#include <ws2tcpip.h>
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
#include <unordered_map>
#include <queue>
#include <utility>
#include <fstream>
#include <functional>
#include <float.h>
#include <typeindex>
#include <cstdint>
#include <algorithm>
using namespace std;

#include "d3dx12.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <DirectXTex.h>

using namespace DirectX;
using namespace Microsoft::WRL;

#include <fmod.hpp>

#include "asio.hpp"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244 4251 4267 4996)
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "DTO.pb.h"
#include "Protocol.pb.h"
#include "ProtocolLib.h"

#ifdef _MSC_VER
#pragma warning(pop)
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

#include "Asio_Network_Library.h"

#include "GPUBuffer.h"

#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3dcompiler.lib")

#pragma comment(lib, "ws2_32.lib")

#pragma comment(lib, "fmod_vc.lib")

#pragma comment(lib, "Asio_Network_Library.lib")

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
		MessageBoxA(NULL, msg, "Assertion Failed", MB_OK); \
        exit(-1); \
	}
#endif				

#define GET(type) type::Get()
#define ENGINE			 GET(Engine)

// Engine class member
#define SCENE_MANAGER    GET(Engine).GetSceneManager()
#define NETWORK_MANAGER  GET(Engine).GetNetworkManager()
#define SOUND_MANAGER    GET(Engine).GetSoundManager()
#define EFFECT_MANAGER   GET(Engine).GetEffectManager()
#define UI_MANAGER       GET(Engine).GetUIManager()

// Singleton based classes
#define RESOURCE		 GET(ResourceManager)
#define IMGUI			 GET(ImGuiManager)
#define TIMER			 GET(Timer)
#define INPUT			 GET(Input)

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

inline XMINT2 WinSize;
constexpr int SWAP_CHAIN_BUFFER_COUNT = 2;
constexpr size_t CONSTANT_BUFFER_ALIGNMENT = 256;

struct Vertex
{
	XMFLOAT3 pos;
	XMFLOAT2 uv;
	XMFLOAT3 normal;
	XMFLOAT3 tangent;
	XMFLOAT4 weights;
	XMFLOAT4 indices;
	XMFLOAT4 color;
};
