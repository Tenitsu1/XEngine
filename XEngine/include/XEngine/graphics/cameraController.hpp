#pragma once

#include "XEngine/graphics/camera.hpp"
#include "XEngine/input/keyboard.h" 
#include "XEngine/input/mouse.h" 

class CameraController
{
public:
    CameraController(Camera& camera);

    bool OnUpdate(float deltaTime);

    inline void SetSpeed(float speed) { mMoveSpeed = speed; }
    inline float GetSpeed() const { return mMoveSpeed; }

private:
    Camera& mCamera; 
    float mMoveSpeed = 5.0f;

    struct KeyBindings {
        int Forward = XENGINE_INPUT_KEY_UP;   
        int Backward = XENGINE_INPUT_KEY_DOWN;
        int Left = XENGINE_INPUT_KEY_LEFT;
        int Right = XENGINE_INPUT_KEY_RIGHT;
        int Up = XENGINE_INPUT_KEY_SPACE;
        int Down = XENGINE_INPUT_KEY_LSHIFT;
    } mKeys;
};
