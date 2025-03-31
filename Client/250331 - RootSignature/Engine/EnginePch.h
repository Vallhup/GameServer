#pragma once

#define _HAS_STD_BYTE 0

// 각종 include
#include <Windows.h>
#include <tchar.h>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <list>
#include <map>
using namespace std;

#include "d3dx12.h"
#include <d3d12.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;


// 각종 lib
#pragma comment(lib, "d3d12")
#pragma comment(lib, "dxgi")
#pragma comment(lib, "dxguid")
#pragma comment(lib, "d3dcompiler")

// 각종 typedef
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;

enum class CBV_REGISTER
{
	b0,
	b1,
	b2,
	b3,
	b4,

	END
};

enum
{
	SWAP_CHAIN_BUFFER_COUNT = 2,
	CBV_REGISTER_COUNT = CBV_REGISTER::END,
	REGISTER_COUNT = CBV_REGISTER::END
};

struct WindowInfo
{
	HWND	hwnd;
	int32	width;
	int32	height;
	bool	windowed;		// 전체화면(false) & 창모드(true)
};

struct Vertex {
	XMFLOAT3 pos;
	XMFLOAT4 color;
};

struct Transform {
	XMFLOAT3 offset;
};

#define DEVICE			GEngine->GetDevice()->GetDevice()
#define CMD_LIST		GEngine->GetCmdQueue()->GetCmdList()
#define ROOT_SIGNATURE	GEngine->GetRootSignature()->GetSignature()

extern unique_ptr<class Engine> GEngine;