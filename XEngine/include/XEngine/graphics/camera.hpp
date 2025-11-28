#pragma once

#include <external/glm/glm.hpp>
#include <external/glm/gtc/matrix_transform.hpp>
#include <vector>


// 定義相機移動方向的枚舉
enum CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

// 這是原本你的 struct，我們將其保留作為傳遞給 Shader 的數據包
// 我稍微整理了一下，確保只包含 Shader 可能需要的數據 (或是與你 setUniformCamera 兼容的格式)
struct CameraData
{
    glm::vec3 position;
    glm::vec3 lookat;   // 在你的邏輯中，這其實是 Front (方向向量)
    glm::vec3 up;

    glm::vec3 direction;
    glm::vec3 right;
    glm::vec3 cameraUp;

    glm::mat4 view;

    float fov;
    float yaw;
    float pitch;

    // 這些選項保留在 struct 內如果你的 shader 不需要它們，可以考慮移出，
    // 但為了兼容你現有的 setUniformCamera，我們先留著
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
};

class Camera
{
public:
    // 相機屬性
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    //尤拉角
    float Yaw;
    float Pitch;

    // 相機選項
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom; // FOV

    // 建構子
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f);

    // 返回 View Matrix
    glm::mat4 GetViewMatrix();

    // 返回 Projection Matrix
    glm::mat4 GetProjectionMatrix(float width, float height, float nearPlane = 0.1f, float farPlane = 100.0f);

    // 處理鍵盤輸入
    void ProcessKeyboard(CameraMovement direction, float deltaTime);

    // 處理滑鼠移動 (視角旋轉)
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    // 處理滾輪 (縮放)
    void ProcessMouseScroll(float yoffset);

    // 取得用於 Shader 的數據結構 (轉換 Class 狀態為 struct)
    CameraData GetShaderData();

private:
    // 根據尤拉角更新向量
    void updateCameraVectors();
};
