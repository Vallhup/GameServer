#pragma once

class WindowInfo
{
public:
	DECLARE_SINGLE(WindowInfo);

	void Init();
	GLFWwindow* GetWindow() { return window; }

private:
	GLFWwindow* window = nullptr;
	const char* basename = "PEW & PEW";
};