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

std::string PickPackage(GLFWwindow* window)
{
    if (NFD_Init() != NFD_OKAY)
        throw std::runtime_error(NFD_GetError());
    struct DialogGuard { ~DialogGuard() { NFD_Quit(); } } guard;
    nfdu8filteritem_t filter = {"Package files", "pkz,pak"};
    nfdopendialogu8args_t args{};
    args.filterList = &filter;
    args.filterCount = 1;
#if defined(__APPLE__) || defined(_WIN32)
    NFD_GetNativeWindowFromGLFWWindow(window, &args.parentWindow);
#else
    (void)window;
#endif
    nfdu8char_t* path = nullptr;
    const nfdresult_t result = NFD_OpenDialogU8_With(&path, &args);
    if (result == NFD_ERROR)
        throw std::runtime_error(NFD_GetError());
    if (result == NFD_CANCEL)
        return {};
    std::unique_ptr<nfdu8char_t, decltype(&NFD_FreePathU8)> ownedPath(path, NFD_FreePathU8);
    return std::string(path);
}
