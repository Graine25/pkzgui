#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include "ui/workspace.h"
#include "ui/app_menu.h"
#include "platform/file_dialog.h"
#include "pkzgui_font.h"

namespace
{
    void ReportGlfwError(int error, const char* description)
    {
        std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
    }
}

int main(int argc, char** argv)
{
    glfwSetErrorCallback(ReportGlfwError);
    if (!glfwInit())
        return 1;

#ifdef __APPLE__
    const char* glslVersion = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    const char* glslVersion = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 800, "pkzgui", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return 1;
    }
    glfwSetWindowSizeLimits(window, 480, 320, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(FontData),
        sizeof(FontData), 13.0f, &fontConfig);
    ImGui::StyleColorsLight();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = style.ChildRounding = style.FrameRounding = 0;
    style.ItemSpacing = ImVec2(8, 0);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(1, 1, 1, 1);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.94f, 0.94f, 0.94f, 1);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.96f, 0.96f, 0.96f, 1);
    style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(1, 1, 1, 1);

    if (!ImGui_ImplGlfw_InitForOpenGL(window, true))
    {
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    if (!ImGui_ImplOpenGL3_Init(glslVersion))
    {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    AppMenu menu;
    if (!menu.Init(window))
    {
        menu.Shutdown();
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    Workspace workspace;
    if (argc > 1)
        workspace.session.BeginLoad(argv[1]);
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (glfwGetWindowAttrib(window, GLFW_ICONIFIED))
        {
            glfwWaitEventsTimeout(0.1);
            continue;
        }
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        MenuAction action = menu.DrawAndPoll();
#ifdef __APPLE__
        const bool openShortcut = io.KeySuper && ImGui::IsKeyPressed(ImGuiKey_O, false);
#else
        const bool openShortcut = io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O, false);
#endif
#ifdef __APPLE__
        const bool saveShortcut = io.KeySuper && ImGui::IsKeyPressed(ImGuiKey_S, false);
#else
        const bool saveShortcut = io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false);
#endif
        if (workspace.approvedOpen || openShortcut) action = MenuAction::OpenPackage;
        else if (saveShortcut) action = MenuAction::Save;
        workspace.HandleAction(action, window);
        workspace.Draw();
        workspace.UpdateMenus(menu);
        ImGui::Render();

        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    workspace.player.Stop();
    workspace.preview.Clear();
    workspace.modelPreview.Clear();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    menu.Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}