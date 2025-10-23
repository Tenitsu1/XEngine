#pragma once

#include <array>

namespace XEngine::input
{
	class Mouse
	{
	public:
		static void initialize();
		static void update();

		inline static float X() { return x; }
		inline static float Y() { return y; }
		inline static float dX() { return x - xLast; }
		inline static float dY() { return y - yLast; }

		static bool button(int button);
		static bool buttonDown(int button);
		static bool buttonUp(int button);


	private:
		constexpr static const int buttonCount = 5; // Since SDL support up to 5 mouse button.
		static float x, xLast;
		static float y, yLast;

		static std::array<bool, buttonCount> buttons;
		static std::array<bool, buttonCount> buttonsLast;
	};

}

enum 
{
	XENGINE_INPUT_MOUSE_FIRST = 1,
	XENGINE_INPUT_MOUSE_LEFT = XENGINE_INPUT_MOUSE_FIRST,
	XENGINE_INPUT_MOUSE_MIDDLE = 2,
	XENGINE_INPUT_MOUSE_RIGHT = 3,
	XENGINE_INPUT_MOUSE_X1 = 4,
	XENGINE_INPUT_MOUSE_X2 = 5,
	XENGINE_INPUT_MOUSE_LAST = 5
};