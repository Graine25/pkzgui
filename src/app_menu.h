#pragma once
#include <array>
#include <cstddef>

struct GLFWwindow;

enum class MenuAction { None, OpenPackage, Save, SaveAs, ExtractTexture, ReplaceTexture, ExtractAudio, PlayAudio, Exit, Count };

class AppMenu
{
public:
    bool Init(GLFWwindow* window);
    MenuAction DrawAndPoll();
    void SetEnabled(MenuAction action, bool enabled);
    void Shutdown(); // Call before destroying the GLFW window.

private:
    std::array<void*, static_cast<std::size_t>(MenuAction::Count)> items{};
    std::array<bool, static_cast<std::size_t>(MenuAction::Count)> enabled{};
    bool mInitialized = false;
};
