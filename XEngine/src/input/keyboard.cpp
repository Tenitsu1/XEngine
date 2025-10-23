#include "input/keyboard.h"
#include "log.h"

#include <algorithm>
#include <SDL3/SDL_keyboard.h>

namespace XEngine::input
{

	std::array<bool, Keyboard::keyCount> Keyboard::keys;
	std::array<bool, Keyboard::keyCount> Keyboard::keysLast;

	void Keyboard::initialize()
	{
		std::fill(keys.begin(), keys.end(), false);
		std::fill(keysLast.begin(), keysLast.end(), false);
	}

	void Keyboard::update()
	{
		
		keysLast = keys;
		const bool* state = SDL_GetKeyboardState(nullptr);

		for (int i = 0; i < keyCount; i++)
		{
			keys[i] = state[i];
		}
	}


	bool Keyboard::key(int key)
	{
		XENGINE_ASSERT(key >= XENGINE_INPUT_KEY_FIRST && key <= XENGINE_INPUT_KEY_LAST, "Invaid Keyboard key");
		if (key >= XENGINE_INPUT_KEY_FIRST && key <= XENGINE_INPUT_KEY_LAST)
		{
			return keys[key];
		}
		return false;
	}

	bool Keyboard::keyDown(int key)
	{
		XENGINE_ASSERT(key >= XENGINE_INPUT_KEY_FIRST && key <= XENGINE_INPUT_KEY_LAST, "Invaid Keyboard key");
		if (key >= XENGINE_INPUT_KEY_FIRST && key <= XENGINE_INPUT_KEY_LAST)
		{
			return keys[key] && !keysLast[key];
		}
		return false;
	}

	bool Keyboard::keyUp(int key)
	{
		XENGINE_ASSERT(key >= XENGINE_INPUT_KEY_FIRST && key <= XENGINE_INPUT_KEY_LAST, "Invaid Keyboard key");
		if (key >= XENGINE_INPUT_KEY_FIRST && key <= XENGINE_INPUT_KEY_LAST)
		{
			return !keys[key] && keysLast[key];
		}
		return false;
	}
}