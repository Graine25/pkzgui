#pragma once
#include <string>
struct GLFWwindow;
std::string PickPackage(GLFWwindow* window);
std::string PickFile(GLFWwindow* window, const char* name, const char* extensions, bool save, const std::string& defaultName = {});
