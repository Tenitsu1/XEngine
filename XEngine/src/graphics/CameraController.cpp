#include "XEngine/graphics/cameraController.hpp"



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
    
    /*if (input::Mouse::isButtonPressed(1)) {
        float dx = input::Mouse::dX();
        float dy = input::Mouse::dY();
        if (dx != 0 || dy != 0) {
            mCamera.ProcessMouseMovement(dx, dy);
            moved = true;
        }
    }*/
    

    return moved;
}
