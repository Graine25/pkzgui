#include "app_menu.h"
#include <cstdio>
#include <string>

#ifdef PKZGUI_NATIVE_MENUS
#include "native_menu_bar.h"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#endif
#else
#include "imgui.h"
#endif

#ifdef __APPLE__
extern "C" void SetMenuShortcut(void* item, const char* key);
#endif

namespace
{
    struct Item
    {
        const char* label;
        const char* shortcut = nullptr;
        MenuAction action = MenuAction::None;
    };
    constexpr Item file[] = {
        {"Open package...", "Ctrl+O", MenuAction::OpenPackage}, {"Save", "Ctrl+S", MenuAction::Save},
        {"Save As...", nullptr, MenuAction::SaveAs}, {"Load schema..."},
#ifndef __APPLE__
        {nullptr}, {"Exit", nullptr, MenuAction::Exit},
#endif
    };
    constexpr Item edit[] = {{"Edit selected field...", "Enter"}};
    constexpr Item view[] = {{"Expand all"}, {"Collapse all"}, {nullptr}, {"Simple view"}};
    constexpr Item texture[] = {{"Extract texture (PNG)...", nullptr, MenuAction::ExtractTexture}, {"Replace texture (PNG)...", nullptr, MenuAction::ReplaceTexture}};
    constexpr Item audio[] = {{"Extract sample (WAV)...", nullptr, MenuAction::ExtractAudio}, {"Play sample", nullptr, MenuAction::PlayAudio}};
    struct Menu
    {
        const char* label;
        const Item* items;
        size_t count;
    };
    template<size_t N>
    constexpr Menu DefineMenu(const char* label, const Item (&items)[N])
    {
        return {label, items, N};
    }
    constexpr Menu menus[] = {
        DefineMenu("File", file), DefineMenu("Edit", edit), DefineMenu("View", view),
        DefineMenu("Texture", texture), DefineMenu("Audio", audio),
    };
}

bool AppMenu::Init(GLFWwindow* window)
{
    enabled[static_cast<size_t>(MenuAction::OpenPackage)] = true;
    enabled[static_cast<size_t>(MenuAction::Exit)] = true;
#ifdef PKZGUI_NATIVE_MENUS
#ifdef _WIN32
    nmb_setup(glfwGetWin32Window(window), nmb_SetupFlags_none);
#else
    (void)window;
    nmb_setup(nullptr, nmb_SetupFlags_none);
#endif
    mInitialized = true;
    int index = 0;
    for (const Menu& definition : menus)
    {
        // Cocoa indices start after the application menu; insert before Window.
        nmb_Handle menu = nmb_insertMenu(nullptr, index++, definition.label);
        if (!menu)
            break;
        for (size_t i = 0; i < definition.count; ++i)
        {
            const Item& item = definition.items[i];
            if (!item.label)
            {
                nmb_appendSeparator(menu);
                continue;
            }
            std::string caption = item.label;
#ifdef _WIN32
            if (item.shortcut)
                caption += std::string("\t") + item.shortcut;
#endif
            nmb_Handle handle = nmb_appendMenuItem(menu, caption.c_str());
            if (!handle)
                break;
            const size_t action = static_cast<size_t>(item.action);
            nmb_setMenuItemEnabled(handle, enabled[action]);
            if (item.action != MenuAction::None) items[action] = handle;
#ifdef __APPLE__
            if (item.action == MenuAction::OpenPackage) SetMenuShortcut(handle, "o");
            if (item.action == MenuAction::Save) SetMenuShortcut(handle, "s");
#endif
        }
    }
    if (*nmb_getLastError())
    {
        std::fprintf(stderr, "Native menu error: %s\n", nmb_getLastError());
        return false;
    }
#else
    (void)window;
#endif
    return true;
}

MenuAction AppMenu::DrawAndPoll()
{
    MenuAction action = MenuAction::None;
#ifdef PKZGUI_NATIVE_MENUS
    nmb_Event event;
    while (nmb_pollEvent(&event))
    {
        if (event.event != nmb_EventType_itemTriggered)
            continue;
        for (size_t i = 1; i < items.size(); ++i)
            if (items[i] == event.sender && enabled[i])
            {
                if (static_cast<MenuAction>(i) == MenuAction::Exit) return MenuAction::Exit;
                action = static_cast<MenuAction>(i);
            }
    }
#else
    // GLFW cannot embed the library's GTK menu widget; retain the Linux menu.
    if (ImGui::BeginMainMenuBar())
    {
        for (const Menu& menu : menus)
        {
            if (!ImGui::BeginMenu(menu.label))
                continue;
            for (size_t i = 0; i < menu.count; ++i)
            {
                const Item& item = menu.items[i];
                if (!item.label)
                    ImGui::Separator();
                else if (ImGui::MenuItem(item.label, item.shortcut, false, enabled[static_cast<size_t>(item.action)]))
                    action = item.action;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
#endif
    return action;
}

void AppMenu::Shutdown()
{
#ifdef PKZGUI_NATIVE_MENUS
    if (mInitialized)
        nmb_shutdown();
#endif
    mInitialized = false;
    items.fill(nullptr);
    enabled.fill(false);
}

void AppMenu::SetEnabled(MenuAction action, bool value)
{
    const size_t index = static_cast<size_t>(action);
    if (enabled[index] == value) return;
    enabled[index] = value;
#ifdef PKZGUI_NATIVE_MENUS
    if (items[index]) nmb_setMenuItemEnabled(items[index], value);
#endif
}
