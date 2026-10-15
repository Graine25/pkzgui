#include "platform/file_io.h"
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <mutex>

std::vector<uint8_t> ReadFile(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::runtime_error("Cannot open file: " + path.u8string());
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)), {});
    if (input.bad())
        throw std::runtime_error("Cannot read file: " + path.u8string());
    return bytes;
}

void WriteFile(const std::filesystem::path& path, const std::vector<uint8_t>& bytes)
{
    std::ofstream output(path, std::ios::binary);
    if (!output)
        throw std::runtime_error("Cannot open file for writing: " + path.u8string());
    output.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    output.close();
    if (!output)
        throw std::runtime_error("Failed writing file: " + path.u8string());
}

std::string SanitizeFileName(std::string name)
{
    if (name.empty()) name = "sample";
    for (char& c : name)
        if (std::string("/\\:*?\"<>|").find(c) != std::string::npos || static_cast<unsigned char>(c) < 32)
            c = '_';
    return name;
}

std::filesystem::path DebugLogPath()
{
#ifdef _WIN32
    const char* root=std::getenv("LOCALAPPDATA");
    return root?std::filesystem::u8path(root)/"GSX"/"logs"/"gsx.log":std::filesystem::path{};
#else
    const char* home=std::getenv("HOME");
    if(!home)return {};
#ifdef __APPLE__
    return std::filesystem::u8path(home)/"Library"/"Logs"/"GSX"/"gsx.log";
#else
    const char* state=std::getenv("XDG_STATE_HOME");
    return (state?std::filesystem::u8path(state):std::filesystem::u8path(home)/".local"/"state")/"gsx"/"gsx.log";
#endif
#endif
}

void AppendDebugLog(const std::string& message)
{
    static std::mutex mutex;
    const std::lock_guard<std::mutex> lock(mutex);
    try {
        const auto path=DebugLogPath();
        if(path.empty())return;
        std::filesystem::create_directories(path.parent_path());
        if(std::filesystem::exists(path) && std::filesystem::file_size(path)>2*1024*1024){
            auto previous=path;previous.replace_filename("gsx.previous.log");
            std::filesystem::remove(previous);std::filesystem::rename(path,previous);
        }
        std::ofstream output(path,std::ios::app);
        const auto now=std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        output<<std::put_time(std::localtime(&now),"%Y-%m-%d %H:%M:%S")<<" "<<message<<"\n";
    }catch(const std::exception&){}
}
