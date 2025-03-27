#pragma once

#include <windows.h>

struct Piece
{
	void KeyInput(WPARAM wp)
	{
		switch (wp) {
		case VK_LEFT:
			if (--xPos < 1) xPos = 1;
			break;

		case VK_RIGHT:
			if (++xPos > 8) xPos = 8;
			break;

		case VK_UP:
			if (--yPos < 1) yPos = 1;
			break;

		case VK_DOWN:
			if (++yPos > 8) yPos = 8;
			break;
		}
	}

	// 1 ~ 8
	int xPos{ 1 };
	int yPos{ 1 };
};