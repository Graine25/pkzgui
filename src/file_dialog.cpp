#include "file_dialog.h"
#include <nfd.h>
#include <memory>
#include <stdexcept>

#ifdef __APPLE__
#define GLFW_EXPOSE_NATIVE_COCOA
#elif defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#if defined(__APPLE__) || defined(_WIN32)
#include <nfd_glfw3.h>
#endif

std::string PickFile(GLFWwindow* window, const char* name, const char* extensions, bool save, const std::string& defaultName)
{
    if (NFD_Init() != NFD_OKAY)
        throw std::runtime_error(NFD_GetError());
    struct DialogGuard { ~DialogGuard() { NFD_Quit(); } } guard;
    nfdu8filteritem_t filter = {name, extensions};
    nfdwindowhandle_t parent{};
#if defined(__APPLE__) || defined(_WIN32)
    NFD_GetNativeWindowFromGLFWWindow(window, &parent);
#else
    (void)window;
#endif
    nfdu8char_t* path = nullptr;
    nfdresult_t result;
    if (save)
    {
        nfdsavedialogu8args_t args{};
        args.filterList = &filter;
        args.filterCount = 1;
        args.parentWindow = parent;
        args.defaultName = defaultName.c_str();
        result = NFD_SaveDialogU8_With(&path, &args);
    }
    else
    {
        nfdopendialogu8args_t args{};
        args.filterList = &filter;
        args.filterCount = 1;
        args.parentWindow = parent;
        result = NFD_OpenDialogU8_With(&path, &args);
    }
    if (result == NFD_ERROR)
        throw std::runtime_error(NFD_GetError());
    if (result == NFD_CANCEL)
        return {};
    std::unique_ptr<nfdu8char_t, decltype(&NFD_FreePathU8)> ownedPath(path, NFD_FreePathU8);
    return std::string(path);
}

std::string PickPackage(GLFWwindow* window)
{
    return PickFile(window, "Package files", "pkz,pak", false);
}
