#pragma once

struct GLFWwindow;

enum class MenuAction { None, OpenPackage, Exit };

class AppMenu
{
public:
    bool Init(GLFWwindow* window);
    MenuAction DrawAndPoll();
    void Shutdown(); // Call before destroying the GLFW window.

private:
    void* mExitItem = nullptr;
    void* mOpenItem = nullptr;
    bool mInitialized = false;
};
