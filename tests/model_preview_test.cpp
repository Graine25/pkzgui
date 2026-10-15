#include "model/model_preview.h"
#include "package/resource_selection.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <array>
#include <cstdio>
#include <set>
#include <stdexcept>

void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }

CMChunk Geometry(const std::string& name, uint32_t count = 3)
{
    auto resource = CMChunk::Container(Gen_PostLoadData, 1);
    resource.AddChild(CMChunkResourceHeader::Build(0, kCMResourceType_Geometry, 0, 0, 0, 0, name));
    auto lod = CMChunk::Container(0x325, 1);
    auto descriptor = CMChunk::Leaf(0x327, 1, {});
    for (uint32_t value : {0u, 0u, 20u, 0u, count, 0u, 3u, 0u, 0u, 0u, 0u, 0u}) descriptor.Append(value);
    auto vertices = CMChunk::Leaf(0x1388, 1, {});
    for (uint32_t i = 0; i < count; ++i) {
        vertices.Append(i == 0 ? -1.f : (i == 1 ? 1.f : 0.f));
        vertices.Append(i == 2 ? 1.f : 0.f);
        vertices.Append(0.f);
        vertices.Append(0x7FC00000u); vertices.Append(0x7FC00000u);
    }
    auto indices = CMChunk::Leaf(0x1389, 1, {});
    for (uint16_t index : {0, 2, 1}) indices.Append(index);
    lod.AddChild(std::move(descriptor)); lod.AddChild(std::move(vertices)); lod.AddChild(std::move(indices));
    resource.AddChild(std::move(lod));
    return resource;
}

PackageSession Fixture()
{
    PackageSession session;
    session.package = std::make_unique<PKPackage>();
    auto& roots = session.package->rootChunks;
    roots.push_back(Geometry("first"));
    roots.push_back(Geometry("second", 4));
    auto missing = CMChunk::Container(GenSub_Resource, 1);
    missing.AddChild(CMChunkResourceHeader::Build(0, kCMResourceType_Geometry, 0, 0, 0, 0, "missing"));
    roots.push_back(std::move(missing));
    auto audio = CMChunk::Container(GenSub_Resource, 1);
    audio.AddChild(CMChunkResourceHeader::Build(0, kCMResourceType_AudioSample, 0, 0, 0, 0, "audio"));
    roots.push_back(std::move(audio));
    session.selected = &roots[0];
    return session;
}

void SelectionTests()
{
    auto session = Fixture();
    ModelPreview preview;
    preview.Update(session);
    Check(preview.active && preview.error.empty() && preview.vertexCount == 3 && preview.triangleCount == 1,
        "Valid padded vertex stride was rejected or decoded incorrectly");
    session.selected = &session.package->rootChunks[1];
    preview.Update(session);
    Check(preview.name == "second" && preview.vertexCount == 4 && preview.error.empty(), "Wrong model selected");
    session.selected = &session.package->rootChunks[2];
    preview.Update(session);
    Check(preview.active && !preview.error.empty() && preview.vertexCount == 0, "Missing model displayed unrelated geometry");
    session.selected = &session.package->rootChunks[3];
    preview.Update(session);
    Check(!preview.active, "Model preview intercepted an audio selection");
    session.selected = nullptr;
    preview.Update(session);
    Check(!preview.active, "Model preview activated without a selection");
    session.selected = &session.package->rootChunks[0];
    session.package->isLittleEndian = true;
    preview.Update(session);
    Check(preview.error.find("big-endian") != std::string::npos, "Unsupported byte order was silently decoded");
    session.package->isLittleEndian = false;
    auto& vertices = session.package->rootChunks[0].children[1].children[1].data;
    vertices[0] = 0x7F; vertices[1] = 0xC0; vertices[2] = vertices[3] = 0;
    ++session.revision;
    preview.Update(session);
    Check(!preview.error.empty() && preview.vertexCount == 0, "Non-finite vertices were accepted");
    std::puts("PASS: geometry selection, padded stride, missing resources, audio, byte order and invalid vertices");
}

template<class T> T Proc(const char* name)
{
    auto proc = reinterpret_cast<T>(glfwGetProcAddress(name));
    Check(proc != nullptr, name);
    return proc;
}

int GLTests()
{
    if (!glfwInit()) return 77;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif
    GLFWwindow* window = glfwCreateWindow(600, 480, "Model regression", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 77; }
    glfwMakeContextCurrent(window);
    ImGui::CreateContext();
    auto& io = ImGui::GetIO(); io.IniFilename = nullptr; io.DisplaySize = ImVec2(600, 480); io.DeltaTime = 1.f / 60;
    unsigned char* pixels; int width, height; io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    const auto genFbo = Proc<void(*)(int, unsigned*)>("glGenFramebuffers");
    const auto bindFbo = Proc<void(*)(unsigned, unsigned)>("glBindFramebuffer");
    const auto deleteFbo = Proc<void(*)(int, const unsigned*)>("glDeleteFramebuffers");
    const auto isFbo = Proc<unsigned char(*)(unsigned)>("glIsFramebuffer");
    const auto isBuffer = Proc<unsigned char(*)(unsigned)>("glIsBuffer");
    unsigned fbo[2]; genFbo(2, fbo);
    bindFbo(0x8CA9, fbo[0]); bindFbo(0x8CA8, fbo[1]);
    glViewport(4, 5, 73, 67); glEnable(GL_SCISSOR_TEST); glScissor(0, 0, 1, 1);
    glEnable(GL_BLEND); glEnable(GL_DEPTH_TEST); glEnable(GL_CULL_FACE);
    glDepthMask(GL_FALSE); glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
    glDepthFunc(GL_GREATER); glCullFace(GL_FRONT); glFrontFace(GL_CW);
    auto session = Fixture(); ModelPreview preview; preview.Update(session);
    const auto draw = [&]() {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(0, 0)); ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Test", nullptr, ImGuiWindowFlags_NoDecoration);
        preview.Draw(); ImGui::End(); ImGui::Render();
        Check(preview.error.empty(), preview.error.c_str());
        int value, viewport[4]; glGetIntegerv(0x8CA6, &value); Check(value == int(fbo[0]), "Draw FBO not restored");
        glGetIntegerv(0x8CAA, &value); Check(value == int(fbo[1]), "Read FBO not restored");
        glGetIntegerv(GL_VIEWPORT, viewport); Check(viewport[0] == 4 && viewport[2] == 73, "Viewport not restored");
        Check(glIsEnabled(GL_SCISSOR_TEST) && glIsEnabled(GL_BLEND) && glIsEnabled(GL_DEPTH_TEST) && glIsEnabled(GL_CULL_FACE), "GL enables not restored");
        GLboolean mask; glGetBooleanv(GL_DEPTH_WRITEMASK, &mask); Check(mask == GL_FALSE, "Depth write mask not restored");
        glGetIntegerv(GL_DEPTH_FUNC, &value); Check(value == GL_GREATER, "Depth function not restored");
        glGetIntegerv(GL_FRONT_FACE, &value); Check(value == GL_CW, "Winding not restored");
        GLboolean colors[4]; glGetBooleanv(GL_COLOR_WRITEMASK, colors);
        Check(!colors[0] && colors[1] && !colors[2] && colors[3], "Color write mask not restored");
        for (unsigned binding : {0x8B8Du, 0x85B5u, 0x8894u, 0x8CA7u, unsigned(GL_TEXTURE_BINDING_2D)}) {
            glGetIntegerv(binding, &value); Check(value == 0, "GL object binding leaked into caller");
        }
        Check(glGetError() == GL_NO_ERROR, "Model preview generated a GL error");
    };
    draw();
    std::set<unsigned> buffers;
    unsigned modelFbo = 0;
    for (unsigned i = 1; i < 64; ++i) {
        if (isBuffer(i)) buffers.insert(i);
        if (isFbo(i) && i != fbo[0] && i != fbo[1]) modelFbo = i;
    }
    Check(buffers.size() == 3 && modelFbo != 0, "Mesh buffers were not retained");
    bindFbo(0x8CA8, modelFbo);
    std::array<unsigned char, 256 * 256 * 4> image{};
    glReadPixels(0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
    bool visible = false;
    for (size_t i = 0; i < image.size(); i += 4) if (image[i] > 90) visible = true;
    Check(visible, "Camera did not show the triangle");
    bindFbo(0x8CA9, modelFbo); glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glClearColor(1, 0, 0, 1); glClear(GL_COLOR_BUFFER_BIT);
    glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
    bindFbo(0x8CA9, fbo[0]); bindFbo(0x8CA8, fbo[1]);
    draw();
    bindFbo(0x8CA8, modelFbo);
    std::array<unsigned char, 4> pixel{}; glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    Check(pixel[0] == 255 && pixel[1] == 0, "Idle preview was rendered again");
    bindFbo(0x8CA8, fbo[1]);
    io.DisplaySize = ImVec2(800, 600); draw();
    for (unsigned id : buffers) Check(isBuffer(id), "Resize reallocated a mesh buffer");
    preview.Clear();
    for (unsigned id : buffers) Check(!isBuffer(id), "Clear leaked a mesh buffer");
    bindFbo(0x8CA9, 0); bindFbo(0x8CA8, 0); deleteFbo(2, fbo);
    ImGui::DestroyContext(); glfwDestroyWindow(window); glfwTerminate();
    std::puts("PASS: visible model, GL state isolation, retained buffers, cached frames, resize and shutdown cleanup");
    return 0;
}

int main(int argc, char**)
{
    try { if (argc > 1) return GLTests(); SelectionTests(); return 0; }
    catch (const std::exception& ex) { std::fprintf(stderr, "%s\n", ex.what()); return 1; }
}
