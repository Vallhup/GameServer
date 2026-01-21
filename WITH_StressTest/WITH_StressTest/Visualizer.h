#pragma once

#include <winsock2.h>
#include <WS2tcpip.h>
#include <mswsock.h>
#include <Windows.h>

#include <gl/GL.h>
#include <gl/GLU.h>

#include <string>
#include <vector>
#include <array>
#include <thread>
#include <mutex>

#include "Client.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

class Visualizer {
	static constexpr std::wstring_view WINDOW_NAME{ L"Graduation Project Test Client Visulizer" };

public:
	Visualizer() = delete;
	Visualizer(int width, int height, bool isFull);
	~Visualizer();

public:
	void Start();
	void Stop();

	void Render();

private:
	void UpdateClientPositions(const std::vector<std::shared_ptr<Client>>& clients);
	void ResizeGLWindow(GLsizei width, GLsizei height);
	void InitOpenGL();
	void BuildFont();
	void glPrint(const char* fmt, ...);
	void KillGLWindow();

	static LRESULT CALLBACK VisulizerProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	HWND _hWnd;
	HDC _hDC;
	HGLRC _hRC;
	HINSTANCE _hInstance;

	bool _active;
	bool _isFull;

	GLuint _base;

	std::mutex _clientMutex;
	std::vector<std::shared_ptr<Client>> _clientSnapshot;
};

