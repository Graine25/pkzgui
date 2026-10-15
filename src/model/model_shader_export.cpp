#include "model/model_material.h"
#include "platform/file_io.h"
#include <sstream>
#include <stdexcept>

namespace {
std::string JSON(const std::string& text)
{
    std::string result = "\"";
    const char* hex = "0123456789abcdef";
    for (unsigned char c : text) {
        if (c == '"' || c == '\\') { result += '\\'; result += char(c); }
        else if (c < 32) { result += "\\u00"; result += hex[c >> 4]; result += hex[c & 15]; }
        else result += char(c);
    }
    return result + '"';
}
}
void ExportModelShaders(const ModelMaterial& material, const std::string& path)
{
    std::vector<uint8_t> binary;
    std::ostringstream manifest;
    manifest << "{\n  \"version\": 1,\n  \"material\": " << JSON(material.name)
             << ",\n  \"materialFlags\": " << material.flags << ",\n  \"samplers\": [";
    for (size_t i = 0; i < material.samplers.size(); ++i) {
        const auto& sampler = material.samplers[i];
        if (i) manifest << ',';
        manifest << "\n    {\"slot\":" << i << ",\"crc\":" << sampler.crc << ",\"addressMode\":" << unsigned(sampler.addressMode)
                 << ",\"sourcePath\":" << JSON(sampler.sourcePath) << '}';
    }
    manifest << "\n  ],\n  \"passes\": [";
    bool first = true;
    for (const auto& pass : material.passes) {
        if (!first) manifest << ',';
        first = false;
        manifest << "\n    {\"slot\":" << pass.slot << ",\"set\":" << pass.set << ",\"format\":" << pass.format;
        for (int stage = 0; stage < 2; ++stage) {
            const auto container = ModelShaderContainer(material, stage ? pass.pixel : pass.vertex);
            manifest << ",\"" << (stage ? "pixel" : "vertex") << "\":{\"offset\":" << binary.size() << ",\"size\":" << container.size() << '}';
            binary.insert(binary.end(), container.begin(), container.end());
        }
        manifest << '}';
    }
    manifest << "\n  ],\n  \"declarations\": [";
    for (size_t i = 0; i < material.declarations.size(); ++i) {
        if (i) manifest << ',';
        const auto& d = material.declarations[i];
        manifest << "\n    {\"format\":" << d.format << ",\"slot\":" << d.slot << ",\"set\":" << d.set << ",\"attributes\":[";
        for (size_t j = 0; j < d.attributes.size(); ++j) {
            if (j) manifest << ',';
            const auto& a = d.attributes[j];
            manifest << "{\"stream\":" << a.stream << ",\"offset\":" << a.offset << ",\"type\":" << a.type << ",\"usage\":" << unsigned(a.usage) << ",\"index\":" << unsigned(a.index) << '}';
        }
        manifest << "]}";
    }
    manifest << "\n  ]\n}\n";
    if (binary.empty()) throw std::runtime_error("This material has no embedded shader containers.");
    WriteFile(std::filesystem::u8path(path), binary);
    const auto json = manifest.str();
    WriteFile(std::filesystem::u8path(path + ".json"), std::vector<uint8_t>(json.begin(), json.end()));
}
