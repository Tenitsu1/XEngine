//#pragma once
//#include "camera.h"
//#include "XEngine/engine.h"
//#include "XEngine/core/window.h"
//#include "XEngine/input/mouse.h"
//#include "XEngine/input/keyboard.h"
//#include <string>
//
//
//namespace XEngine::graphics
//{
//    class CameraHandler
//    {
//    public:
//        CameraHandler(Camera& camera);
//
//        static void SDLKeyCallback(SDL_Window* mWindow, int key, int scancode, int action, int mods) {
//            CameraHandler* cameraHandler = reinterpret_cast<CameraHandler*>(Engine::Instance().getWindow().getSDLWindow());
//            if (cameraHandler) {
//                cameraHandler->keyCallback(mWindow, key, scancode, action, mods);
//            }
//        }
//
//        static void SDLMousePositionCallback(SDL_Window* mWindow, double xpos, double ypos) {
//            CameraHandler* cameraHandler = reinterpret_cast<CameraHandler*>(Engine::Instance().getWindow().getSDLWindow());
//            if (cameraHandler) {
//                cameraHandler->mouseCursorPositionCallback(mWindow, xpos, ypos);
//            }
//        }
//
//        void mouseCursorPositionCallback(SDL_Window* mWindow, double xpos, double ypos);
//        void keyCallback(SDL_Window* mWindow, int key, int scancode, int action, int mods);
//
//    private:
//        Camera& camera;
//
//    public:
//        bool CameraControllMode = false;
//    };
//}