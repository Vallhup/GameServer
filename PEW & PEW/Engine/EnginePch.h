#pragma once

// 각종 include
#define _CRT_SECURE_NO_WARNINGS

#include <winsock2.h>    // Windows 소켓 API - 이 헤더가 windows.h보다 앞에 존재해야함
#include <ws2tcpip.h>	 // TCP / IP
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <utility>
#include <array>
#include "protocol.h"
using namespace std;

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/DefaultLogger.hpp>

#include <fmod.hpp>

// 각종 lib
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable: 4711 4710 4100)

#ifdef _DEBUG
#pragma comment(lib, "assimp-vc143-mtd.lib")
#else
#pragma comment(lib, "assimp-vc143-mt.lib")
#endif

#pragma comment(lib, "fmod_vc.lib")

// 각종 typedef
using int8 = __int8;
using int16 = __int16;
using int32 = __int32;
using int64 = __int64;
using uint8 = unsigned __int8;
using uint16 = unsigned __int16;
using uint32 = unsigned __int32;
using uint64 = unsigned __int64;

enum class PlayerPVPState {
	WAITING,
	READY,
	FIGHT,
	WIN,
	LOSE
};

// 싱글톤 매크로
#define DECLARE_SINGLE(type)		\
private:							\
	type() {}						\
	~type() {}						\
public:								\
	static type* GetInstance()		\
	{								\
		static type instance;		\
		return &instance;			\
	}								\

#define GET_SINGLE(type)	type::GetInstance()

#define CAMERAPOS()			GET_SINGLE(Camera)->getPosition()

// 공용 변수들
#define MAX_NUM_BONES_PER_VERTEX 4

extern unsigned int WIN_W;		// 2560
extern unsigned int WIN_H;		// 1440
extern unsigned int WIN_X;
extern unsigned int WIN_Y;

constexpr float PI = { 3.141592f };

extern double cur_x;
extern double cur_y;

extern glm::vec3 mouseDir;

constexpr unsigned int MAX_BONES = { 100 };

extern float light_angle;

// 공용 함수들
string LoadFile(const string& filename);
GLuint LoadTexture(const char* path);
void SetupShader(const char* vertexName, const char* fragmentName, GLuint& shaderName);