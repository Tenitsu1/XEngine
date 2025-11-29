#pragma once

class Camera;

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
};
