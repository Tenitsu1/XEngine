#include "graphics/camera.hpp"



Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(5.0f), MouseSensitivity(0.1f), Zoom(45.0f)
{
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix()
{
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 Camera::GetProjectionMatrix(float width, float height, float nearPlane, float farPlane)
{
    return glm::perspective(glm::radians(Zoom), width / height, nearPlane, farPlane);
}

void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime)
{
    // 根據你的邏輯，這裡使用 deltaTime * MovementSpeed
    // 注意：你在 main 中原本有一個 deltaTimeMax 用於加速，
    // 建議在呼叫此函數時，將 (speed * multiplier) 整合進 deltaTime 或是修改 MovementSpeed

    float velocity = MovementSpeed * deltaTime;

    if (direction == FORWARD)
        Position += Front * velocity;
    if (direction == BACKWARD)
        Position -= Front * velocity;
    if (direction == LEFT)
        Position -= Right * velocity;
    if (direction == RIGHT)
        Position += Right * velocity;
    if (direction == UP)
        Position += WorldUp * velocity; // 垂直向上 (Space)
    if (direction == DOWN)
        Position -= WorldUp * velocity; // 垂直向下 (LShift)
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch -= yoffset; // 注意：通常 yoffset 往上是正，但 pitch 往上看通常要增加或減少取決於座標系，這裡假設減法

    if (constrainPitch)
    {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset)
{
    Zoom -= (float)yoffset;
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 89.0f)
        Zoom = 89.0f;
}

void Camera::updateCameraVectors()
{
    // 計算新的 Front 向量
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);

    // 重新計算 Right 和 Up
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up = glm::normalize(glm::cross(Right, Front));
}

CameraData Camera::GetShaderData()
{
    CameraData data;
    data.position = Position;
    data.lookat = Front; // 這裡對應你原本 code 中的 lookat (方向)
    data.up = Up;
    data.direction = Front;
    data.right = Right;
    data.cameraUp = Up;
    data.view = GetViewMatrix();
    data.fov = Zoom;
    data.yaw = Yaw;
    data.pitch = Pitch;
    data.MovementSpeed = MovementSpeed;
    data.MouseSensitivity = MouseSensitivity;
    data.Zoom = Zoom;
    return data;
}
