#include "input/mouse.h"
#include "log.h"

#include <algorithm>
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_events.h"

namespace XEngine::input
{
	SDL_Event event;
	float Mouse::x = 0;
	float Mouse::xLast = 0;
	float Mouse::y = 0;
	float Mouse::yLast = 0;
	float Mouse::mouseWheelx = 0;
	float Mouse::mouseWheely = 0;

	std::array<bool, Mouse::buttonCount> Mouse::buttons;
	std::array<bool, Mouse::buttonCount> Mouse::buttonsLast;

	void Mouse::initialize()
	{
		std::fill(buttons.begin(), buttons.end(), false);
		std::fill(buttonsLast.begin(), buttonsLast.end(), false);
	}

	void Mouse::update()
	{
		xLast = x;
		yLast = y;
		buttonsLast = buttons; 
		UINT32 state = SDL_GetMouseState(&x, &y);

		for (int i = 0; i < buttonCount; i++)
		{
			buttons[i] = state & SDL_BUTTON_MASK(i + 1);
		}
	}


	bool Mouse::button(int button)
	{
		XENGINE_ASSERT(button >= XENGINE_INPUT_MOUSE_FIRST && button <= XENGINE_INPUT_MOUSE_LAST, "Invaid mouse button");
		if (button >= XENGINE_INPUT_MOUSE_FIRST && button <= XENGINE_INPUT_MOUSE_LAST)
		{
			return buttons[button -1];
		}
		return false;
	}

	bool Mouse::buttonDown(int button)
	{
		XENGINE_ASSERT(button >= XENGINE_INPUT_MOUSE_FIRST && button <= XENGINE_INPUT_MOUSE_LAST, "Invaid mouse button");
		if (button >= XENGINE_INPUT_MOUSE_FIRST && button <= XENGINE_INPUT_MOUSE_LAST)
		{
			return buttons[button -1] && !buttonsLast[button - 1];
		}
		return false;
	}

	bool Mouse::buttonUp(int button)
	{
		XENGINE_ASSERT(button >= XENGINE_INPUT_MOUSE_FIRST && button <= XENGINE_INPUT_MOUSE_LAST, "Invaid mouse button");
		if (button >= XENGINE_INPUT_MOUSE_FIRST && button <= XENGINE_INPUT_MOUSE_LAST)
		{
			return !buttons[button - 1] && buttonsLast[button - 1];
		}
		return false;
	}
}