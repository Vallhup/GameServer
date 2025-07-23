#pragma once

class WindowInfo
{
	DECLARE_SINGLE(WindowInfo);

public:
	void Init();
	GLFWwindow* GetWindow() { return window; }

private:
	GLFWwindow* window = { nullptr };
	const char* basename = { "PEW & PEW" };
};