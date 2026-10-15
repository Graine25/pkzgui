#include "model/model_preview.h"
#include "package/resource_selection.h"
#include "imgui.h"
#include "texture/texture_actions.h"
#include "texture/cube_texture.h"
#include "platform/file_io.h"
#include "BaseUtils/BUPng.h"
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

CMChunk TestMaterial(uint32_t textureCRC)
{
    auto chunk = CMChunk::Leaf(0x331, 64, std::vector<uint8_t>(244));
    const auto put = [&](size_t offset, uint32_t value) {
        for (int i = 0; i < 4; ++i) chunk.data[offset+i] = uint8_t(value >> (24 - 8*i));
    };
    std::memcpy(chunk.data.data(), "fixture", 7);
    for (size_t offset : {100,104,108,112}) put(offset, 1);
    chunk.Append(textureCRC); chunk.Append<uint32_t>(0); chunk.Append<uint32_t>(0);
    chunk.AppendBytes(std::string(128, 'x').data(), 128);
    for (int i = 0; i < 4; ++i) chunk.Append(1.f);
    chunk.Append(0.f); chunk.Append<uint32_t>(0x00000000); chunk.Append<uint32_t>(0x01000000); chunk.Append<uint32_t>(0);
    const size_t pass = chunk.data.size(); chunk.data.resize(pass + 188); put(pass, 0x42);
    chunk.Append<uint32_t>(88);
    const size_t decl = chunk.data.size(); chunk.data.resize(decl + 88); put(decl + 24, 3);
    for (int i = 0; i < 3; ++i) {
        const size_t at = decl + 52 + i * 12;
        put(at, i == 0 ? 4 : (i == 1 ? 16 : 24));
        put(at+4, i == 0 ? 0x2a23b9 : (i == 1 ? 0x2a2190 : 0x2c2359));
        chunk.data[at+9] = i == 0 ? 0 : (i == 1 ? 3 : 5);
        chunk.data[at+10] = i == 2 ? 1 : 0;
    }
    for (int shader = 0; shader < 2; ++shader) { chunk.Append<uint32_t>(1); chunk.Append<uint32_t>(0); chunk.Append<uint32_t>(0); }
    return chunk;
}

PackageSession TexturedFixture()
{
    auto session = Fixture();
    auto& resource = session.package->rootChunks[0];
    auto material = CMChunk::Container(0x321, 3); material.AddChild(TestMaterial(0));
    resource.AddChild(std::move(material));
    auto& lod = resource.children[1];
    auto& desc = lod.children[0];
    desc.data.clear();
    for (uint32_t value : {0u,0u,32u,0x42u,3u,0u,3u,0u,0u,0u,0u,0u}) desc.Append(value);
    auto& vb = lod.children[1]; vb.data.clear();
    for (int i = 0; i < 3; ++i) {
        vb.Append(77.f);
        vb.Append(i == 0 ? -1.f : (i == 1 ? 1.f : 0.f)); vb.Append(i == 2 ? 1.f : 0.f); vb.Append(0.f);
        vb.Append<uint32_t>(0x7FC00000); vb.Append<uint32_t>(0);
        vb.Append<int16_t>(i == 1 ? 1024 : 0); vb.Append<int16_t>(i == 2 ? 1024 : 0); vb.Append<uint32_t>(0);
    }
    std::vector<uint8_t> pixels(16*16*4, 0);
    for (size_t i = 0; i < pixels.size(); i += 4) { pixels[i+1] = 255; pixels[i+3] = 0; }
    const auto texture = RZTexture::FromRGBA8("fixture diffuse", pixels.data(),16,16);
    resource.children.back().children[0] = TestMaterial(texture.nameCRC);
    auto root = CMChunk::Container(Root,1); auto library = CMChunk::Container(Gen_TextureLibrary,1);
    library.AddChild(texture.BuildLibraryResource(0x123400)); root.AddChild(std::move(library));
    auto post = texture.BuildPostLoadData(0x123400); post.offset = 0x123400; root.AddChild(std::move(post));
    session.package->rootChunks.push_back(std::move(root)); session.selected = &session.package->rootChunks[0];
    return session;
}

CMChunk CubeFixture()
{
    auto cube=CMChunk::Leaf(Texture_Data,7,{});
    const auto format=XenosTexture::D3DFormat::Decode(XenosTexture::D3DFormat::kA8R8G8B8Tiled);
    for(uint32_t value:{1u,1u,64u,1u,0u,format.raw,0u,0u,0u,0u,0u,0u,0u})cube.Append(value);
    for(unsigned face=0;face<6;++face){
        std::vector<uint8_t> rgba(64*64*4);
        for(size_t i=0;i<rgba.size();i+=4){
            rgba[i]=face?255:0;rgba[i+1]=face?0:255;rgba[i+2]=face?255:0;rgba[i+3]=255;
        }
        const auto linear=XenosTexture::EncodeRGBA8(rgba.data(),64,64,format);
        const auto tiled=XenosTexture::Tile(linear.data(),64,64,format);
        cube.AppendBytes(tiled.data(),tiled.size());
    }
    return cube;
}

void CubeTests()
{
    auto data=CubeFixture();const auto cube=DecodeCubeTexture(data);
    Check(cube.size==64,"Cube header size was read as a 2D descriptor");
    for(unsigned face=0;face<6;++face){
        const auto& rgba=cube.faces[face];
        Check(rgba.size()==64*64*4,"Cube face size is wrong");
        Check(face?rgba[0]>240 && rgba[1]<10:rgba[0]<10 && rgba[1]>240,"Cube face offset or pixel format is wrong");
    }
    data.data.pop_back();bool rejected=false;
    try{DecodeCubeTexture(data);}catch(const std::exception&){rejected=true;}
    Check(rejected,"Truncated cube face data was accepted");
    std::puts("PASS: cube header, six face offsets, pixel format and truncation");
}

void MaterialTests()
{
    auto session = TexturedFixture(); ModelPreview preview; preview.Update(session);
    Check(preview.error.empty(), preview.error.c_str());
    Check(preview.Materials().size() == 1 && preview.Materials()[0].texture && preview.Materials()[0].texture->GetName() == "fixture diffuse", "Material CRC did not resolve its texture");
    const auto& mesh = preview.Meshes().at(0);
    Check(mesh.positions[0] == -1.f && mesh.positions[1] == 0.f && mesh.positions[3] == 1.f, "Declaration position offset was ignored");
    Check(mesh.hasUV && mesh.texcoords[2] == 1024.f && mesh.texcoords[5] == 1024.f, "UV set 1 or integer short2 was decoded incorrectly");
    Check(mesh.normals[2] > 0.99f, "Packed normal was decoded incorrectly");
    Check(mesh.hasShaderUV[1] && !mesh.hasShaderUV[0] && mesh.shaderUV[1][2]==1024.f,
        "Guest UV1 was lost or substituted for UV0");
    auto& material = session.package->rootChunks[0].children.back().children[0];
    material.data.pop_back(); ++session.revision; preview.Update(session);
    Check(preview.error.find("Truncated") != std::string::npos, "Truncated shader record was accepted");
    material = TestMaterial(0); material.data[244+140+23] = 255; ++session.revision; preview.Update(session);
    Check(preview.error.empty() && !preview.Materials()[0].texture, "Missing diffuse sampler selected another texture");
    auto image = CMChunk::Leaf(0,1,std::vector<uint8_t>(0x368+40));
    const auto put = [&](size_t at, uint32_t value) { for (int i=0; i<4; ++i) image.data[at+i] = uint8_t(value >> (24-8*i)); };
    put(0x368,0x102a1101); put(0x36c,36); put(0x370,4); put(0x368+36,0x12345678);
    ModelShaderRecord record{0,0x368+36,0x368+36,4,0x368};
    ModelMaterial exported; exported.name = "test\\shader\"\n"; exported.source = &image;
    exported.passes.push_back({0,0,1,record,{}});
    const auto container = ModelShaderContainer(exported,record);
    Check(container.size() == 40 && container[0] == 0x10 && container[36] == 0x12, "Shader export did not concatenate the virtual and physical container");
    const auto exportPath = (std::filesystem::temp_directory_path()/"pkzgui-model-shader-test.bin").u8string();
    ExportModelShaders(exported,exportPath);
    Check(ReadFile(exportPath) == container, "Exported shader bytes differ");
    const auto json = ReadFile(exportPath+".json");
    Check(std::string(json.begin(),json.end()).find("\\u000a") != std::string::npos, "Shader binding manifest contains an unescaped control character");
    std::filesystem::remove(exportPath); std::filesystem::remove(exportPath+".json");
    image.data.pop_back(); bool rejected = false;
    try { ModelShaderContainer(exported,record); } catch (const std::exception&) { rejected = true; }
    Check(rejected, "Truncated shader physical data was exported");
    std::puts("PASS: material CRCs, declarations, UV sets, packed normals, missing samplers and truncated shaders");
}

void ReferenceTests()
{
    auto inlineModel = Geometry("same name");
    auto descriptors = inlineModel.children[1];
    auto buffers = descriptors;
    descriptors.children.erase(descriptors.children.begin()+1,descriptors.children.end());
    descriptors.children[0].version = 8;
    buffers.children.erase(buffers.children.begin());
    auto info = CMChunk::Leaf(0x326,6,std::vector<uint8_t>(24));
    descriptors.AddChild(info); buffers.AddChild(info);
    auto geometry = CMChunk::Container(0x321,3); geometry.AddChild(std::move(descriptors));
    auto library = CMChunk::Container(GenSub_Resource,1);
    library.AddChild(CMChunkResourceHeader::Build(11,kCMResourceType_Geometry,31,0,0x2000,0,"same name"));
    library.AddChild(std::move(geometry));
    auto post = CMChunk::Container(Gen_PostLoadData,1); post.offset = 0x2000;
    post.AddChild(library.children[0]); post.AddChild(std::move(buffers));
    auto decoy = post; decoy.offset = 0x1000;
    decoy.children[0] = CMChunkResourceHeader::Build(22,kCMResourceType_Geometry,31,0,0x1000,0,"same name");
    auto root = CMChunk::Container(Root,1); root.AddChild(std::move(library)); root.AddChild(std::move(decoy)); root.AddChild(std::move(post));
    PackageSession session; session.package = std::make_unique<PKPackage>(); session.package->rootChunks.push_back(std::move(root));
    auto& children = session.package->rootChunks[0].children;
    session.selected = &children[0]; ModelPreview preview; preview.Update(session);
    Check(preview.error.empty() && preview.vertexCount == 3, "Resource-offset geometry lookup failed");
    children[0].children[0] = CMChunkResourceHeader::Build(11,kCMResourceType_Geometry,31,0,0x1000,0,"same name");
    ++session.revision; preview.Update(session);
    Check(preview.error.find("does not match") != std::string::npos, "A same-name model at the wrong resource offset was used");
    children[0].children[0] = CMChunkResourceHeader::Build(11,kCMResourceType_Geometry,31,0,0x2000,0,"same name");
    children[2].children[1].children.back().data[15] = 1;
    ++session.revision; preview.Update(session);
    Check(preview.error.find("LOD") != std::string::npos, "Mismatched LOD descriptors and buffers were paired");
    std::puts("PASS: exact resource offsets, same-name collisions and matching LOD indices");
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
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
    Check(buffers.size() == 10 && modelFbo != 0, "Mesh buffers were not retained");
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
    auto textured = TexturedFixture();
    preview.Update(textured); preview.SetTextured(true); draw();
    unsigned texturedFbo = 0;
    for (unsigned i = 1; i < 256; ++i) if (isFbo(i) && i != fbo[0] && i != fbo[1]) texturedFbo = i;
    Check(texturedFbo != 0, "Textured preview has no framebuffer");
    const auto colorCount = [&](bool green) {
        bindFbo(0x8CA8, texturedFbo);
        GLint attachment, imageW, imageH;
        Proc<void(*)(unsigned,unsigned,unsigned,int*)>("glGetFramebufferAttachmentParameteriv")(0x8CA8,0x8CE0,0x8CD1,&attachment);
        glBindTexture(GL_TEXTURE_2D,attachment);
        glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_WIDTH,&imageW); glGetTexLevelParameteriv(GL_TEXTURE_2D,0,GL_TEXTURE_HEIGHT,&imageH);
        glBindTexture(GL_TEXTURE_2D,0);
        std::vector<uint8_t> rgba(size_t(imageW)*imageH*4);
        glReadPixels(0,0,imageW,imageH,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
        bindFbo(0x8CA8, fbo[1]);
        size_t count = 0;
        for (size_t i=0; i<rgba.size(); i+=4) {
            if (green ? rgba[i+1] > rgba[i]+40 && rgba[i+1] > rgba[i+2]+40 : rgba[i] > rgba[i+1]+40 && rgba[i+2] > rgba[i+1]+40) ++count;
        }
        return count;
    };
    Check(colorCount(true) > 100, "Package texture missing, UV not uploaded, or non-opacity alpha discarded the mesh");
    const auto dir = std::filesystem::temp_directory_path() / "pkzgui-model-preview-test";
    std::filesystem::create_directories(dir);
    const auto png = dir / "override.png";
    const std::vector<uint8_t> replacement = {255,0,255,255};
    WriteFile(png, BUPng::EncodeRGBA8(replacement.data(),1,1));
    const auto selected = textured.selected;
    const auto before = RZTexture::Load(*textured.package, *preview.Materials()[0].texture).DecodeLevel0();
    const auto activeTexture = Proc<void(*)(unsigned)>("glActiveTexture");
    unsigned sentinel; glGenTextures(1,&sentinel); activeTexture(0x84C3); glBindTexture(GL_TEXTURE_2D,sentinel);
    glPixelStorei(GL_UNPACK_ALIGNMENT,8); glPixelStorei(0x0CF2,9); glPixelStorei(0x0CF3,2); glPixelStorei(0x0CF4,1);
    preview.LoadPNG(0,png.u8string());
    GLint saved; glGetIntegerv(0x84E0,&saved); Check(saved == 0x84C3, "PNG upload changed the active texture unit");
    glGetIntegerv(GL_TEXTURE_BINDING_2D,&saved); Check(saved == GLint(sentinel), "PNG upload changed another texture unit's binding");
    glGetIntegerv(0x0CF2,&saved); Check(saved == 9, "PNG upload leaked pixel unpack state");
    glPixelStorei(GL_UNPACK_ALIGNMENT,4); glPixelStorei(0x0CF2,0); glPixelStorei(0x0CF3,0); glPixelStorei(0x0CF4,0);
    glDeleteTextures(1,&sentinel); activeTexture(0x84C0); draw();
    Check(colorCount(false) > 100, "PNG override did not reach the material sampler");
    Check(!textured.dirty && textured.selected == selected && before == RZTexture::Load(*textured.package,*preview.Materials()[0].texture).DecodeLevel0(), "Preview-only override changed the package");
    preview.UsePackageTexture(0); draw();
    Check(colorCount(true) > 100, "Use package texture did not restore the original image");
    preview.UseRetexture(0); draw();
    Check(colorCount(false) > 100 && !textured.dirty, "Switching back to the retexture lost the PNG or modified the package");
    preview.SetTextured(false); draw();
    Check(colorCount(false) < 100 && !textured.dirty, "Texture visibility toggle did not hide the retexture");
    preview.SetTextured(true); draw();
    Check(colorCount(false) > 100, "Texture visibility toggle lost the active retexture");
    preview.ClearRetexture(0); draw();
    Check(colorCount(true) > 100, "Clearing a retexture did not restore the package texture");
    bool missingRetexture = false;
    try { preview.UseRetexture(0); } catch (const std::exception&) { missingRetexture = true; }
    Check(missingRetexture, "A cleared retexture remained selectable");
    preview.LoadPNG(0,png.u8string());
    const uint8_t edited[4] = {0,255,0,255};
    WriteFile(png,BUPng::EncodeRGBA8(edited,1,1));
    bool needsReload = false;
    try { preview.ApplyPNG(textured,0); } catch (const std::exception&) { needsReload = true; }
    Check(needsReload && !textured.dirty, "A PNG edited after preview was applied without reloading");
    preview.LoadPNG(0,png.u8string()); draw();
    Check(colorCount(true) > 100, "Reloading an edited PNG did not refresh the preview");
    WriteFile(png,BUPng::EncodeRGBA8(replacement.data(),1,1));
    preview.LoadPNG(0,png.u8string()); preview.ApplyPNG(textured,0);
    Check(textured.dirty && textured.selected == selected, "Applying a model texture changed selection or failed to mark dirty");
    const auto changed = RZTexture::Load(*textured.package,*preview.Materials()[0].texture).DecodeLevel0();
    Check(changed != before && changed[0] > 240 && changed[1] < 10 && changed[2] > 240, "Applied texture did not retain the replacement pixels");
    preview.Update(textured); draw();
    Check(colorCount(false) > 100, "Package edit invalidated the model or reset the textured mode");
    // Guest replacements must keep the last valid program and isolate all touched GL state.
    auto& materialBytes = textured.package->rootChunks[0].children.back().children[0].data;
    materialBytes[244 + 140 + 24] = 0;
    ++textured.revision; preview.Update(textured);
    ShaderAsset guest;
    guest.vertexGLSL = R"(#version 150
in vec3 gsxPosition;
layout(std140) uniform type_GsxVertexBlock { vec4 registers[256]; };
void main() {
    vec4 p = vec4(gsxPosition, 1.0);
    gl_Position = vec4(dot(p,registers[0]),dot(p,registers[1]),dot(p,registers[2]),dot(p,registers[3]));
})";
    guest.pixelGLSL = "#version 150\nout vec4 color; void main(){color=vec4(1,0,1,1);}";
    for (uint32_t i=0;i<4;++i) guest.constants.push_back({false,i,1,"kVS_ViewProj"+std::to_string(i)});
    const auto shaderPath = (dir / "replacement.gsxshader").u8string();
    WriteShaderAsset(guest,shaderPath);
    preview.LoadShaderPreview(0,shaderPath);
    unsigned textures[2], uniformBuffers[3]; glGenTextures(2,textures);
    const auto bindBuffer = Proc<void(*)(unsigned,unsigned)>("glBindBuffer");
    const auto bindBase = Proc<void(*)(unsigned,unsigned,unsigned)>("glBindBufferBase");
    const auto getIndexed = Proc<void(*)(unsigned,unsigned,int*)>("glGetIntegeri_v");
    Proc<void(*)(int,unsigned*)>("glGenBuffers")(3,uniformBuffers);
    for (unsigned i=0;i<3;++i) bindBase(0x8A11,i,uniformBuffers[i]);
    for (unsigned i=0;i<2;++i) { activeTexture(0x84C0+(i?15:2)); glBindTexture(GL_TEXTURE_2D,textures[i]); }
    activeTexture(0x84C0); draw();
    for (unsigned i=0;i<3;++i) { getIndexed(0x8A28,i,&saved); Check(saved==int(uniformBuffers[i]),"Guest shader leaked a UBO binding"); }
    glGetIntegerv(0x8A28,&saved); Check(saved==int(uniformBuffers[2]),"Guest shader leaked the global UBO binding");
    for (unsigned i=0;i<2;++i) {
        activeTexture(0x84C0+(i?15:2)); glGetIntegerv(GL_TEXTURE_BINDING_2D,&saved);
        Check(saved==int(textures[i]),"Guest shader leaked a texture binding");
    }
    activeTexture(0x84C0); Check(colorCount(false)>100,"Guest shader replacement did not draw");
    auto broken=guest; broken.pixelGLSL="invalid shader source";
    WriteShaderAsset(broken,shaderPath);
    bool rejected=false;
    try { preview.LoadShaderPreview(0,shaderPath); } catch (const std::exception&) { rejected=true; }
    Check(rejected,"Invalid replacement shader was accepted"); draw();
    Check(colorCount(false)>100,"A failed replacement discarded the last valid shader");
    if (ShaderCompilerAvailable()) {
        guest.vertexHLSL=R"(#define constantRow(INDEX) select((INDEX)<256, vk::RawBufferLoad<float4>(g_PushConstants.VertexShaderConstants + ((INDEX)*16), 0x10), float4(0,0,0,0))
        float4 shaderMain(float3 p : POSITION0) : SV_Position {
            float4 v=float4(p,1);
            return float4(dot(v,constantRow(0)),dot(v,constantRow(1)),dot(v,constantRow(2)),dot(v,constantRow(3))); })";
        guest.pixelHLSL="float4 shaderMain() : SV_Target0 {return float4(0,1,0,1);}";
        WriteShaderAsset(guest,shaderPath);
        const auto edited=CompileShaderEdits(guest,shaderPath+".vert.hlsl",shaderPath+".frag.hlsl");
        WriteShaderAsset(edited,shaderPath); preview.LoadShaderPreview(0,shaderPath); draw();
        Check(colorCount(true)>100,"HLSL edits did not reach the guest preview");
        guest.vertexHLSL=R"(struct Output {float4 position : SV_Position;float2 uv : TEXCOORD0;};
Output shaderMain(float3 p : POSITION0,float2 uv : TEXCOORD1) {
    Output result;float4 v=float4(p,1);
    result.position=float4(dot(v,gsxVS[0]),dot(v,gsxVS[1]),dot(v,gsxVS[2]),dot(v,gsxVS[3]));
    result.uv=uv/1024.0;return result;
})";
        guest.pixelHLSL="float4 shaderMain(float2 uv : TEXCOORD0) : SV_Target0 {return uv.x>0.25?float4(0,1,0,1):float4(1,0,1,1);}";
        WriteShaderAsset(guest,shaderPath);
        const auto secondary=CompileShaderEdits(guest,shaderPath+".vert.hlsl",shaderPath+".frag.hlsl");
        WriteShaderAsset(secondary,shaderPath);preview.LoadShaderPreview(0,shaderPath);draw();
        Check(colorCount(true)>100 && colorCount(false)>100,"Converted TEXCOORD1 shader did not receive the separate UV1 buffer");
        guest.pixelHLSL=R"(Texture2D<float4> g_Texture2DDescriptorHeap[] : register(t0,space0);
TextureCube<float4> g_TextureCubeDescriptorHeap[] : register(t0,space2);
SamplerState g_SamplerDescriptorHeap[] : register(s0,space3);
float4 shaderMain() : SV_Target0 {
    return 0.5*(g_Texture2DDescriptorHeap[0].Sample(g_SamplerDescriptorHeap[0],float2(0.5,0.5))+
        g_TextureCubeDescriptorHeap[1].Sample(g_SamplerDescriptorHeap[1],float3(1,0,0)));
})";
        WriteShaderAsset(guest,shaderPath);
        const auto mixed=CompileShaderEdits(guest,shaderPath+".vert.hlsl",shaderPath+".frag.hlsl");
        ShaderProgram program;program.Load(mixed);
        Check(program.textures.size()==2 && program.textures[0].unit!=program.textures[1].unit &&
            program.textures[0].target!=program.textures[1].target,"2D/cube compiler bindings conflict");
        program.Clear();
    }
    auto cubeRoot=CMChunk::Container(Root,1);
    auto library=CMChunk::Container(Gen_TextureLibrary,1);
    auto cubeResource=CMChunk::Container(GenSub_Resource,1);
    auto header=CMChunkResourceHeader::Build(0x12345678,4,0x1f,0,0x987600,0,"fixture cube");
    cubeResource.AddChild(header);library.AddChild(std::move(cubeResource));cubeRoot.AddChild(std::move(library));
    auto post=CMChunk::Container(Gen_PostLoadData,1);post.offset=0x987600;post.AddChild(header);post.AddChild(CubeFixture());
    cubeRoot.AddChild(std::move(post));textured.package->rootChunks.push_back(std::move(cubeRoot));
    auto& samplerBytes=textured.package->rootChunks[0].children.back().children[0].data;
    for(int i=0;i<4;++i)samplerBytes[244+i]=uint8_t(0x12345678u>>(24-8*i));
    textured.selected=&textured.package->rootChunks[0];++textured.revision;preview.Update(textured);
    guest.pixelGLSL="#version 150\nuniform samplerCube gsxTextureCube_0; out vec4 color; void main(){color=texture(gsxTextureCube_0,vec3(1,0,0));}";
    WriteShaderAsset(guest,shaderPath);preview.LoadShaderPreview(0,shaderPath);
    unsigned sentinelCube;glGenTextures(1,&sentinelCube);activeTexture(0x84C0);glBindTexture(0x8513,sentinelCube);
    draw();glGetIntegerv(0x8514,&saved);Check(saved==int(sentinelCube),"Cube binding leaked into caller");
    Check(colorCount(true)>100,"Cube +X face did not reach the shader");
    guest.pixelGLSL="#version 150\nuniform samplerCube gsxTextureCube_0; out vec4 color; void main(){color=texture(gsxTextureCube_0,vec3(-1,0,0));}";
    WriteShaderAsset(guest,shaderPath);preview.LoadShaderPreview(0,shaderPath);draw();
    Check(colorCount(false)>100,"Cube -X face or sampler binding is wrong");
    glDeleteTextures(1,&sentinelCube);
    auto bytes=ReadFile(shaderPath); bytes.pop_back(); WriteFile(shaderPath,bytes);
    rejected=false; try { ReadShaderAsset(shaderPath); } catch (const std::exception&) { rejected=true; }
    Check(rejected,"Truncated shader asset was accepted");
    for (unsigned i=0;i<3;++i) bindBase(0x8A11,i,0);
    bindBuffer(0x8A11,0); Proc<void(*)(int,const unsigned*)>("glDeleteBuffers")(3,uniformBuffers);
    glDeleteTextures(2,textures);
    preview.Clear(); std::filesystem::remove_all(dir);
    bindFbo(0x8CA9, 0); bindFbo(0x8CA8, 0); deleteFbo(2, fbo);
    ImGui::DestroyContext(); glfwDestroyWindow(window); glfwTerminate();
    std::puts("PASS: visible model, GL state isolation, retained buffers, cached frames, resize, textured rendering, PNG preview/reset/apply, guest replacement, failed compile retention, HLSL edits and shutdown cleanup");
    return 0;
}

int main(int argc, char**)
{
    try { if (argc > 1) return GLTests(); SelectionTests(); MaterialTests(); ReferenceTests(); CubeTests(); return 0; }
    catch (const std::exception& ex) { std::fprintf(stderr, "%s\n", ex.what()); return 1; }
}
