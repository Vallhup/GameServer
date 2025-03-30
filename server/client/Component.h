#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <tchar.h>
#include <algorithm>

#include "Macro.h"

class RenderComponent
{
public:
	void Init(const HDC& hdc, const WCHAR* bitmap)
	{
		if (not _image)
			_image = new Gdiplus::Image(bitmap);

		if(not _graphics)
			_graphics = new Gdiplus::Graphics(hdc);
	}

	void Draw(const HDC& hdc, const int& xPos, const int& yPos, const int& xSize, const int& ySize) const
	{
		_graphics->DrawImage(_image, xPos, yPos, xSize, ySize);
	}

private:
	Gdiplus::Image* _image{ nullptr };
	Gdiplus::Graphics* _graphics{ nullptr };
};