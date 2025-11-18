#pragma once

#include <array>
union SDL_Event;

namespace XEngine::input
{
	class Mouse
	{
	public:
		static void initialize();
		static void update();

		inline static float X() { return x; }
		inline static float Y() { return y; }
		inline static float dX() { return dx; }
		inline static float dY() { return dy; }

		inline static float mouseWheelX() { return mouseWheelx; }
		inline static float mouseWheelY() { return mouseWheely; }

		static bool button(int button);
		static bool buttonDown(int button);
		static bool buttonUp(int button);

		static void ProcessMouseEvent(const SDL_Event& event);

	private:
		constexpr static const int buttonCount = 5; // Since SDL support up to 5 mouse button.
		static float x, xLast;
		static float y, yLast;

		static float dx;
		static float dy;

		static float mouseWheelx;
		static float mouseWheely;

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