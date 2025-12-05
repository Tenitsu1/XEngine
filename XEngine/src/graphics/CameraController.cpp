#include "graphics/cameraController.hpp"

#include "graphics/camera.hpp"
#include "input/keyboard.h" 
#include "input/mouse.h" 

    struct KeyBindings {
        int Forward = XENGINE_INPUT_KEY_W;   
        int Backward = XENGINE_INPUT_KEY_S;
        int Left = XENGINE_INPUT_KEY_A;
        int Right = XENGINE_INPUT_KEY_D;
        int Up = XENGINE_INPUT_KEY_SPACE;
        int Down = XENGINE_INPUT_KEY_LSHIFT;
    } mKeys;

CameraController::CameraController(Camera& camera)
    : mCamera(camera)
{
}

bool CameraController::OnUpdate(float deltaTime)
{
    bool moved = false;

    float currentSpeed = mMoveSpeed;


    if (XEngine::input::Keyboard::key(mKeys.Forward)) {
        mCamera.ProcessKeyboard(CameraMovement::FORWARD, deltaTime * currentSpeed);
        moved = true;
    }
    if (XEngine::input::Keyboard::key(mKeys.Backward)) {
        mCamera.ProcessKeyboard(CameraMovement::BACKWARD, deltaTime * currentSpeed);
        moved = true;
    }
    if (XEngine::input::Keyboard::key(mKeys.Left)) {
        mCamera.ProcessKeyboard(CameraMovement::LEFT, deltaTime * currentSpeed);
        moved = true;
    }
    if (XEngine::input::Keyboard::key(mKeys.Right)) {
        mCamera.ProcessKeyboard(CameraMovement::RIGHT, deltaTime * currentSpeed);
        moved = true;
    }
    if (XEngine::input::Keyboard::key(mKeys.Up)) {
        mCamera.ProcessKeyboard(CameraMovement::UP, deltaTime * currentSpeed);
        moved = true;
    }
    if (XEngine::input::Keyboard::key(mKeys.Down)) {
        mCamera.ProcessKeyboard(CameraMovement::DOWN, deltaTime * currentSpeed);
        moved = true;
    }

    if (XEngine::input::Mouse::button(XENGINE_INPUT_MOUSE_MIDDLE)) {
        float dx = XEngine::input::Mouse::dX();
        float dy = XEngine::input::Mouse::dY();

        if (dx != 0 || dy != 0) {
            float sensitivity = 1.f;
            mCamera.ProcessMouseMovement(dx * sensitivity, dy * sensitivity);
            moved = true;
        }
    }

    if (XEngine::input::Mouse::mouseWheelY() != 0.0f)
    {
        mCamera.ProcessMouseScroll(XEngine::input::Mouse::mouseWheelY());
        moved = true;
    }
    

    return moved;
}
