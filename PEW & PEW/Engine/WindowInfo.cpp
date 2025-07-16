#include "pch.h"
#include "WindowInfo.h"
#include "Input.h"

unsigned int WIN_W = 800;
unsigned int WIN_H = 600;
unsigned int WIN_X = 0;
unsigned int WIN_Y = 0;

void WindowInfo::Init()
{
	glfwInit();

	// 전체화면
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	WIN_W = mode->width;
	WIN_H = mode->height;

	window = glfwCreateWindow(mode->width, mode->height, basename, monitor, NULL);
	
	// 창모드
	//window = glfwCreateWindow(WIN_W, WIN_H, basename, NULL, NULL);

	glfwSetWindowPos(window, WIN_X, WIN_Y);
	glfwMakeContextCurrent(window);
	glewInit();
	glEnable(GL_DEPTH_TEST);

	glClearColor(1.0F, 1.0F, 1.0F, 1.0F);
}
