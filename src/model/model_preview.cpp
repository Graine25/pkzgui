#include "model/model_preview.h"
#include "model/model_geometry.h"
#include "package/resource_selection.h"
#include "imgui.h"
#include "texture/texture_actions.h"
#include "platform/file_dialog.h"
#include "platform/file_io.h"
#include "BaseUtils/BUPng.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <stdexcept>
#include <vector>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#ifndef GL_DEPTH_COMPONENT24
#define GL_DEPTH_COMPONENT24 0x81A6
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_DEPTH_ATTACHMENT
#define GL_DEPTH_ATTACHMENT 0x8D00
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER 0x8D41
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_ELEMENT_ARRAY_BUFFER
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING 0x8CA6
#endif

using GLenum = unsigned int;
using GLuint = unsigned int;
using GLint = int;
using GLsizei = int;
using GLboolean = unsigned char;
using GLchar = char;
using GLsizeiptr = ptrdiff_t;

namespace gl3 {

    using PFN_glCreateShader = GLuint (*)(GLenum);
    using PFN_glShaderSource = void (*)(GLuint, GLsizei, const GLchar* const*, const GLint*);
    using PFN_glCompileShader = void (*)(GLuint);
    using PFN_glGetShaderiv = void (*)(GLuint, GLenum, GLint*);
    using PFN_glGetShaderInfoLog = void (*)(GLuint, GLsizei, GLsizei*, GLchar*);
    using PFN_glDeleteShader = void (*)(GLuint);
    using PFN_glCreateProgram = GLuint (*)(void);
    using PFN_glAttachShader = void (*)(GLuint, GLuint);
    using PFN_glBindAttribLocation = void (*)(GLuint, GLuint, const GLchar*);
    using PFN_glLinkProgram = void (*)(GLuint);
    using PFN_glGetProgramiv = void (*)(GLuint, GLenum, GLint*);
    using PFN_glGetProgramInfoLog = void (*)(GLuint, GLsizei, GLsizei*, GLchar*);
    using PFN_glDeleteProgram = void (*)(GLuint);
    using PFN_glUseProgram = void (*)(GLuint);
    using PFN_glGetUniformLocation = GLint (*)(GLuint, const GLchar*);
    using PFN_glUniformMatrix4fv = void (*)(GLint, GLsizei, GLboolean, const float*);
    using PFN_glUniform1i = void (*)(GLint, GLint);
    using PFN_glUniform1f = void (*)(GLint, float);
    using PFN_glActiveTexture = void (*)(GLenum);
    using PFN_glUniform3f = void (*)(GLint, float, float, float);
    using PFN_glGenBuffers = void (*)(GLsizei, GLuint*);
    using PFN_glBindBuffer = void (*)(GLenum, GLuint);
    using PFN_glBufferData = void (*)(GLenum, GLsizeiptr, const void*, GLenum);
    using PFN_glDeleteBuffers = void (*)(GLsizei, const GLuint*);
    using PFN_glEnableVertexAttribArray = void (*)(GLuint);
    using PFN_glDisableVertexAttribArray = void (*)(GLuint);
    using PFN_glVertexAttribPointer = void (*)(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
    using PFN_glGenVertexArrays = void (*)(GLsizei, GLuint*);
    using PFN_glBindVertexArray = void (*)(GLuint);
    using PFN_glDeleteVertexArrays = void (*)(GLsizei, const GLuint*);
    using PFN_glGenFramebuffers = void (*)(GLsizei, GLuint*);
    using PFN_glBindFramebuffer = void (*)(GLenum, GLuint);
    using PFN_glDeleteFramebuffers = void (*)(GLsizei, const GLuint*);
    using PFN_glFramebufferTexture2D = void (*)(GLenum, GLenum, GLenum, GLuint, GLint);
    using PFN_glCheckFramebufferStatus = GLenum (*)(GLenum);
    using PFN_glGenRenderbuffers = void (*)(GLsizei, GLuint*);
    using PFN_glBindRenderbuffer = void (*)(GLenum, GLuint);
    using PFN_glDeleteRenderbuffers = void (*)(GLsizei, const GLuint*);
    using PFN_glRenderbufferStorage = void (*)(GLenum, GLenum, GLsizei, GLsizei);
    using PFN_glFramebufferRenderbuffer = void (*)(GLenum, GLenum, GLenum, GLuint);

    static PFN_glCreateShader CreateShader = nullptr;
    static PFN_glShaderSource ShaderSource = nullptr;
    static PFN_glCompileShader CompileShaderFn = nullptr;
    static PFN_glGetShaderiv GetShaderiv = nullptr;
    static PFN_glGetShaderInfoLog GetShaderInfoLog = nullptr;
    static PFN_glDeleteShader DeleteShader = nullptr;
    static PFN_glCreateProgram CreateProgram = nullptr;
    static PFN_glAttachShader AttachShader = nullptr;
    static PFN_glBindAttribLocation BindAttribLocation = nullptr;
    static PFN_glLinkProgram LinkProgram = nullptr;
    static PFN_glGetProgramiv GetProgramiv = nullptr;
    static PFN_glGetProgramInfoLog GetProgramInfoLog = nullptr;
    static PFN_glDeleteProgram DeleteProgram = nullptr;
    static PFN_glUseProgram UseProgram = nullptr;
    static PFN_glGetUniformLocation GetUniformLocation = nullptr;
    static PFN_glUniformMatrix4fv UniformMatrix4fv = nullptr;
    static PFN_glUniform1i Uniform1i = nullptr;
    static PFN_glUniform1f Uniform1f = nullptr;
    static PFN_glActiveTexture ActiveTexture = nullptr;
    static PFN_glUniform3f Uniform3f = nullptr;
    static PFN_glGenBuffers GenBuffers = nullptr;
    static PFN_glBindBuffer BindBuffer = nullptr;
    static PFN_glBufferData BufferData = nullptr;
    static PFN_glDeleteBuffers DeleteBuffers = nullptr;
    static PFN_glEnableVertexAttribArray EnableVertexAttribArray = nullptr;
    static PFN_glDisableVertexAttribArray DisableVertexAttribArray = nullptr;
    static PFN_glVertexAttribPointer VertexAttribPointer = nullptr;
    static PFN_glGenVertexArrays GenVertexArrays = nullptr;
    static PFN_glBindVertexArray BindVertexArray = nullptr;
    static PFN_glDeleteVertexArrays DeleteVertexArrays = nullptr;
    static PFN_glGenFramebuffers GenFramebuffers = nullptr;
    static PFN_glBindFramebuffer BindFramebuffer = nullptr;
    static PFN_glDeleteFramebuffers DeleteFramebuffers = nullptr;
    static PFN_glFramebufferTexture2D FramebufferTexture2D = nullptr;
    static PFN_glCheckFramebufferStatus CheckFramebufferStatus = nullptr;
    static PFN_glGenRenderbuffers GenRenderbuffers = nullptr;
    static PFN_glBindRenderbuffer BindRenderbuffer = nullptr;
    static PFN_glDeleteRenderbuffers DeleteRenderbuffers = nullptr;
    static PFN_glRenderbufferStorage RenderbufferStorage = nullptr;
    static PFN_glFramebufferRenderbuffer FramebufferRenderbuffer = nullptr;

    static bool loaded = false;

    template<class T> static void Load(T& function, const char* name)
    {
        function = reinterpret_cast<T>(glfwGetProcAddress(name));
        if (!function) throw std::runtime_error(std::string("Missing OpenGL entry point: ") + name);
    }

    static void EnsureLoaded()
    {
        if (loaded) return;
        Load(CreateShader, "glCreateShader");
        Load(ShaderSource, "glShaderSource");
        Load(CompileShaderFn, "glCompileShader");
        Load(GetShaderiv, "glGetShaderiv");
        Load(GetShaderInfoLog, "glGetShaderInfoLog");
        Load(DeleteShader, "glDeleteShader");
        Load(CreateProgram, "glCreateProgram");
        Load(AttachShader, "glAttachShader");
        Load(BindAttribLocation, "glBindAttribLocation");
        Load(LinkProgram, "glLinkProgram");
        Load(GetProgramiv, "glGetProgramiv");
        Load(GetProgramInfoLog, "glGetProgramInfoLog");
        Load(DeleteProgram, "glDeleteProgram");
        Load(UseProgram, "glUseProgram");
        Load(GetUniformLocation, "glGetUniformLocation");
        Load(UniformMatrix4fv, "glUniformMatrix4fv");
        Load(Uniform3f, "glUniform3f");
        Load(Uniform1i, "glUniform1i");
        Load(Uniform1f, "glUniform1f");
        Load(ActiveTexture, "glActiveTexture");
        Load(GenBuffers, "glGenBuffers");
        Load(BindBuffer, "glBindBuffer");
        Load(BufferData, "glBufferData");
        Load(DeleteBuffers, "glDeleteBuffers");
        Load(EnableVertexAttribArray, "glEnableVertexAttribArray");
        Load(DisableVertexAttribArray, "glDisableVertexAttribArray");
        Load(VertexAttribPointer, "glVertexAttribPointer");
        Load(GenVertexArrays, "glGenVertexArrays");
        Load(BindVertexArray, "glBindVertexArray");
        Load(DeleteVertexArrays, "glDeleteVertexArrays");
        Load(GenFramebuffers, "glGenFramebuffers");
        Load(BindFramebuffer, "glBindFramebuffer");
        Load(DeleteFramebuffers, "glDeleteFramebuffers");
        Load(FramebufferTexture2D, "glFramebufferTexture2D");
        Load(CheckFramebufferStatus, "glCheckFramebufferStatus");
        Load(GenRenderbuffers, "glGenRenderbuffers");
        Load(BindRenderbuffer, "glBindRenderbuffer");
        Load(DeleteRenderbuffers, "glDeleteRenderbuffers");
        Load(RenderbufferStorage, "glRenderbufferStorage");
        Load(FramebufferRenderbuffer, "glFramebufferRenderbuffer");

        loaded = true;
    }

}

namespace {
    enum : GLenum {
        DrawFramebuffer = 0x8CA9, ReadFramebuffer = 0x8CA8,
        DrawFramebufferBinding = 0x8CA6, ReadFramebufferBinding = 0x8CAA,
        CurrentProgram = 0x8B8D, VertexArrayBinding = 0x85B5,
        ArrayBufferBinding = 0x8894, RenderbufferBinding = 0x8CA7
    };

    struct ModelGLState
    {
        GLint drawFbo, readFbo, viewport[4], program, vao, buffer, texture, renderbuffer;
        GLint depthFunc, cullMode, frontFace, activeTexture;
        GLfloat clearColor[4];
        GLdouble clearDepth;
        GLboolean depth, cull, scissor, blend, depthMask, colorMask[4];
        ModelGLState()
        {
            glGetIntegerv(DrawFramebufferBinding, &drawFbo);
            glGetIntegerv(ReadFramebufferBinding, &readFbo);
            glGetIntegerv(GL_VIEWPORT, viewport);
            glGetIntegerv(CurrentProgram, &program);
            glGetIntegerv(VertexArrayBinding, &vao);
            glGetIntegerv(ArrayBufferBinding, &buffer);
            glGetIntegerv(0x84E0, &activeTexture);
            gl3::ActiveTexture(0x84C0);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
            glGetIntegerv(RenderbufferBinding, &renderbuffer);
            glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
            glGetIntegerv(GL_CULL_FACE_MODE, &cullMode);
            glGetIntegerv(GL_FRONT_FACE, &frontFace);
            glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColor);
            glGetDoublev(GL_DEPTH_CLEAR_VALUE, &clearDepth);
            depth = glIsEnabled(GL_DEPTH_TEST); cull = glIsEnabled(GL_CULL_FACE);
            scissor = glIsEnabled(GL_SCISSOR_TEST); blend = glIsEnabled(GL_BLEND);
            glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);
            glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);
        }
        ~ModelGLState()
        {
            gl3::BindFramebuffer(DrawFramebuffer, drawFbo);
            gl3::BindFramebuffer(ReadFramebuffer, readFbo);
            glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
            gl3::UseProgram(program);
            gl3::BindVertexArray(vao);
            gl3::BindBuffer(GL_ARRAY_BUFFER, buffer);
            glBindTexture(GL_TEXTURE_2D, texture);
            gl3::ActiveTexture(activeTexture);
            gl3::BindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
            glDepthFunc(depthFunc); glCullFace(cullMode); glFrontFace(frontFace);
            glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);
            glClearDepth(clearDepth);
            glDepthMask(depthMask);
            glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
            const auto restore = [](GLenum cap, GLboolean enabled) { if (enabled) glEnable(cap); else glDisable(cap); };
            restore(GL_DEPTH_TEST, depth); restore(GL_CULL_FACE, cull);
            restore(GL_SCISSOR_TEST, scissor); restore(GL_BLEND, blend);
        }
    };
}

#ifdef __APPLE__
static const char* kVertexShader = R"(#version 150
in vec3 aPos;
in vec3 aNrm;
in vec2 aUV;
uniform float uUVDivisor;
out vec2 vUV;
uniform mat4 uMVP;
uniform mat4 uModel;
out vec3 vNrm;
out vec3 vWorld;
void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorld = world.xyz;
    vUV = aUV / uUVDivisor;
    vNrm = mat3(uModel) * aNrm;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

static const char* kFragmentShader = R"(#version 150
in vec3 vNrm;
in vec3 vWorld;
in vec2 vUV;
uniform sampler2D uTexture;
uniform bool uTextured;
uniform bool uAlphaCutout;
uniform vec3 uLightDir;
uniform vec3 uViewPos;
uniform vec3 uColor;
out vec4 fragColor;
void main() {
    vec3 n = normalize(vNrm);
    vec3 l = normalize(-uLightDir);
    float diff = max(dot(n, l), 0.0);
    vec3 viewDir = normalize(uViewPos - vWorld);
    vec3 halfDir = normalize(l + viewDir);
    float spec = pow(max(dot(n, halfDir), 0.0), 32.0);
    vec4 albedo = uTextured ? texture(uTexture, vUV) : vec4(1.0);
    if (uTextured && uAlphaCutout && albedo.a < 0.3) discard;
    vec3 base = uColor * albedo.rgb;
    vec3 ambient = 0.35 * base;
    vec3 diffuse = 0.65 * diff * base;
    vec3 specular = 0.25 * spec * vec3(1.0);
    fragColor = vec4(ambient + diffuse + specular, 1.0);
}
)";
#else
static const char* kVertexShader = R"(#version 130
in vec3 aPos;
in vec3 aNrm;
in vec2 aUV;
uniform float uUVDivisor;
out vec2 vUV;
uniform mat4 uMVP;
uniform mat4 uModel;
out vec3 vNrm;
out vec3 vWorld;
void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorld = world.xyz;
    vUV = aUV / uUVDivisor;
    vNrm = mat3(uModel) * aNrm;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

static const char* kFragmentShader = R"(#version 130
in vec3 vNrm;
in vec3 vWorld;
in vec2 vUV;
uniform sampler2D uTexture;
uniform bool uTextured;
uniform bool uAlphaCutout;
uniform vec3 uLightDir;
uniform vec3 uViewPos;
uniform vec3 uColor;
out vec4 fragColor;
void main() {
    vec3 n = normalize(vNrm);
    vec3 l = normalize(-uLightDir);
    float diff = max(dot(n, l), 0.0);
    vec3 viewDir = normalize(uViewPos - vWorld);
    vec3 halfDir = normalize(l + viewDir);
    float spec = pow(max(dot(n, halfDir), 0.0), 32.0);
    vec4 albedo = uTextured ? texture(uTexture, vUV) : vec4(1.0);
    if (uTextured && uAlphaCutout && albedo.a < 0.3) discard;
    vec3 base = uColor * albedo.rgb;
    vec3 ambient = 0.35 * base;
    vec3 diffuse = 0.65 * diff * base;
    vec3 specular = 0.25 * spec * vec3(1.0);
    fragColor = vec4(ambient + diffuse + specular, 1.0);
}
)";
#endif

namespace {

    unsigned int CompileShader(unsigned int type, const char* src)
    {
        gl3::EnsureLoaded();
        unsigned int s = gl3::CreateShader(type);
        gl3::ShaderSource(s, 1, &src, nullptr);
        gl3::CompileShaderFn(s);
        int ok = 0;
        gl3::GetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            char log[512]{};
            gl3::GetShaderInfoLog(s, sizeof(log), nullptr, log);
            gl3::DeleteShader(s);
            throw std::runtime_error(std::string("Shader compile: ") + log);
        }
        return s;
    }

    void Mat4Identity(float* m)
    {
        std::memset(m, 0, 16 * sizeof(float));
        m[0] = m[5] = m[10] = m[15] = 1.f;
    }

    void Mat4Perspective(float* m, float fovyRad, float aspect, float znear, float zfar)
    {
        const float f = 1.f / std::tan(fovyRad * 0.5f);
        std::memset(m, 0, 16 * sizeof(float));
        m[0] = f / aspect;
        m[5] = f;
        m[10] = (zfar + znear) / (znear - zfar);
        m[11] = -1.f;
        m[14] = (2.f * zfar * znear) / (znear - zfar);
    }

    void Mat4LookAt(float* m, float eyeX, float eyeY, float eyeZ,
        float cX, float cY, float cZ, float upX, float upY, float upZ)
    {
        float fx = cX - eyeX, fy = cY - eyeY, fz = cZ - eyeZ;
        float len = std::sqrt(fx * fx + fy * fy + fz * fz);
        if (len > 1e-8f) { fx /= len; fy /= len; fz /= len; }
        float sx = fy * upZ - fz * upY;
        float sy = fz * upX - fx * upZ;
        float sz = fx * upY - fy * upX;
        len = std::sqrt(sx * sx + sy * sy + sz * sz);
        if (len > 1e-8f) { sx /= len; sy /= len; sz /= len; }
        float ux = sy * fz - sz * fy;
        float uy = sz * fx - sx * fz;
        float uz = sx * fy - sy * fx;
        m[0] = sx; m[4] = sy; m[8] = sz; m[12] = 0;
        m[1] = ux; m[5] = uy; m[9] = uz; m[13] = 0;
        m[2] = -fx; m[6] = -fy; m[10] = -fz; m[14] = 0;
        m[3] = 0;  m[7] = 0;  m[11] = 0;  m[15] = 1;
        m[12] = -(sx * eyeX + sy * eyeY + sz * eyeZ);
        m[13] = -(ux * eyeX + uy * eyeY + uz * eyeZ);
        m[14] = -(-fx * eyeX - fy * eyeY - fz * eyeZ);
    }

    void Mat4Mul(float* r, const float* a, const float* b)
    {
        float t[16];
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row)
            {
                t[col * 4 + row] =
                    a[0 * 4 + row] * b[col * 4 + 0] +
                    a[1 * 4 + row] * b[col * 4 + 1] +
                    a[2 * 4 + row] * b[col * 4 + 2] +
                    a[3 * 4 + row] * b[col * 4 + 3];
            }
        std::memcpy(r, t, sizeof(t));
    }

}

ModelPreview::~ModelPreview() { Clear(); }

void ModelPreview::DestroyGL()
{
    if (!gl3::loaded) {
        colorTex = depthRbo = fbo = program = 0;
        fboW = fboH = 0;
        return;
    }
    for (const auto& mesh : gpuMeshes) {
        gl3::DeleteVertexArrays(1, &mesh.vao);
        const unsigned int buffers[] = {mesh.positions, mesh.normals, mesh.indices, mesh.texcoords};
        gl3::DeleteBuffers(4, buffers);
    }
    gpuMeshes.clear();
    for (auto& texture : materialTextures) { if (texture.texture) glDeleteTextures(1, &texture.texture); texture.texture = 0; }
    textureBytes = 0;
    if (whiteTexture) glDeleteTextures(1, &whiteTexture);
    whiteTexture = 0;
    if (colorTex) glDeleteTextures(1, &colorTex);
    if (depthRbo) gl3::DeleteRenderbuffers(1, &depthRbo);
    if (fbo) gl3::DeleteFramebuffers(1, &fbo);
    if (program) gl3::DeleteProgram(program);
    colorTex = depthRbo = fbo = program = 0;
    fboW = fboH = 0;
}

void ModelPreview::Clear()
{
    DestroyGL();
    renderDirty = true;
    meshes.clear();
    materials.clear();
    materialTextures.clear();
    materialIndex = 0;
    textureStatus.clear();
    active = false;
    name.clear();
    error.clear();
    meshCount = vertexCount = triangleCount = 0;
    hasBBox = false;
    selected = nullptr;
    package = nullptr;
}

void ModelPreview::EnsureProgram()
{
    if (program) return;
    gl3::EnsureLoaded();
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, kVertexShader);
    unsigned int fs = 0;
    try { fs = CompileShader(GL_FRAGMENT_SHADER, kFragmentShader); }
    catch (...) { gl3::DeleteShader(vs); throw; }
    program = gl3::CreateProgram();
    gl3::AttachShader(program, vs);
    gl3::AttachShader(program, fs);
    gl3::BindAttribLocation(program, 0, "aPos");
    gl3::BindAttribLocation(program, 1, "aNrm");
    gl3::BindAttribLocation(program, 2, "aUV");
    gl3::LinkProgram(program);
    gl3::DeleteShader(vs);
    gl3::DeleteShader(fs);
    int ok = 0;
    gl3::GetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok)
    {
        char log[512]{};
        gl3::GetProgramInfoLog(program, sizeof(log), nullptr, log);
        gl3::DeleteProgram(program);
        program = 0;
        throw std::runtime_error(std::string("Program link: ") + log);
    }
}

void ModelPreview::EnsureFBO(int w, int h)
{
    gl3::EnsureLoaded();
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    if (fbo && fboW == w && fboH == h) return;
    if (whiteTexture) glDeleteTextures(1, &whiteTexture);
    whiteTexture = 0;
    if (colorTex) glDeleteTextures(1, &colorTex);
    if (depthRbo) gl3::DeleteRenderbuffers(1, &depthRbo);
    if (fbo) gl3::DeleteFramebuffers(1, &fbo);
    colorTex = depthRbo = fbo = 0;

    gl3::GenFramebuffers(1, &fbo);
    gl3::BindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &colorTex);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    gl3::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);

    gl3::GenRenderbuffers(1, &depthRbo);
    gl3::BindRenderbuffer(GL_RENDERBUFFER, depthRbo);
    gl3::RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    gl3::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRbo);

    if (gl3::CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Incomplete framebuffer for model preview.");
    gl3::BindFramebuffer(GL_FRAMEBUFFER, 0);
    fboW = w;
    fboH = h;
}

void ModelPreview::ComputeBounds()
{
    hasBBox = false;
    if (meshes.empty()) return;
    float mn[3] = {1e30f, 1e30f, 1e30f};
    float mx[3] = {-1e30f, -1e30f, -1e30f};
    for (const auto& m : meshes)
        for (size_t i = 0; i + 2 < m.positions.size(); i += 3)
        {
            for (int c = 0; c < 3; ++c)
            {
                mn[c] = std::min(mn[c], m.positions[i + c]);
                mx[c] = std::max(mx[c], m.positions[i + c]);
            }
        }
    if (mn[0] > mx[0]) return;
    std::memcpy(bboxMin, mn, sizeof(mn));
    std::memcpy(bboxMax, mx, sizeof(mx));
    centerX = 0.5f * (mn[0] + mx[0]);
    centerY = 0.5f * (mn[1] + mx[1]);
    centerZ = 0.5f * (mn[2] + mx[2]);
    float dx = mx[0] - mn[0], dy = mx[1] - mn[1], dz = mx[2] - mn[2];
    float radius = 0.5f * std::sqrt(dx * dx + dy * dy + dz * dz);
    distance = std::max(radius * 2.2f, 0.5f);
    hasBBox = true;
}

bool ModelPreview::DecodeGeometry(const PackageSession& session)
{
    meshes.clear();
    if (!session.package) return false;

    std::string want;
    std::optional<CMChunkResourceHeader> resource;
    if (session.selected)
    {
        resource = SelectedResource(*session.package, session.selected);
        if (resource)
        {
            want = resource->GetName();
            name = want;
        }
    }

    if (resource) materials = LoadModelMaterials(*session.package, *resource);
    materialTextures.resize(materials.size());
    if (!resource) return false;
    meshes = LoadModelGeometry(*session.package, *resource, materials);
    if (meshes.empty()) throw std::runtime_error("No drawable mesh in this geometry resource.");

    meshCount = meshes.size();
    vertexCount = 0;
    triangleCount = 0;
    for (const auto& m : meshes)
    {
        vertexCount += m.positions.size() / 3;
        triangleCount += m.indices.size() / 3;
    }
    ComputeBounds();
    return true;
}

void ModelPreview::Update(const PackageSession& session)
{
    if (selected == session.selected && package == session.package.get() && revision == session.revision)
        return;
    const bool sameModel = selected == session.selected && package == session.package.get();
    const float oldYaw = yaw, oldPitch = pitch, oldDistance = distance;
    const int oldMaterial = materialIndex;
    Clear();
    selected = session.selected;
    package = session.package.get();
    revision = session.revision;
    if (!package || !selected) return;
    try
    {
        const auto resource = SelectedResource(*package, selected);
        if (!resource || resource->GetResourceType() != kCMResourceType_Geometry) return;
        active = true;
        name = resource->GetName();
        if (name.empty()) throw std::runtime_error("Geometry resource has no name; cannot match its buffers.");
        if (package->isLittleEndian)
            throw std::runtime_error("Model preview currently supports big-endian geometry only.");
        if (DecodeGeometry(session)) {
            active = true;
            if (sameModel) { yaw = oldYaw; pitch = oldPitch; distance = oldDistance; materialIndex = std::clamp(oldMaterial, 0, std::max(0, int(materials.size()) - 1)); }
        }
    }
    catch (const std::exception& ex)
    {
        active = true;
        error = ex.what();
    }
}

void ModelPreview::UploadMeshes()
{
    gpuMeshes.reserve(meshes.size());
    for (const auto& mesh : meshes)
    {
        gpuMeshes.emplace_back();
        auto& gpu = gpuMeshes.back();
        gl3::GenVertexArrays(1, &gpu.vao);
        gl3::BindVertexArray(gpu.vao);
        gl3::GenBuffers(1, &gpu.positions);
        gl3::GenBuffers(1, &gpu.normals);
        gl3::GenBuffers(1, &gpu.indices);
        gl3::GenBuffers(1, &gpu.texcoords);

        gl3::BindBuffer(GL_ARRAY_BUFFER, gpu.positions);
        gl3::BufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(mesh.positions.size() * sizeof(float)),
            mesh.positions.data(), GL_STATIC_DRAW);
        gl3::EnableVertexAttribArray(0);
        gl3::VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        gl3::BindBuffer(GL_ARRAY_BUFFER, gpu.normals);
        gl3::BufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(mesh.normals.size() * sizeof(float)),
            mesh.normals.data(), GL_STATIC_DRAW);
        gl3::EnableVertexAttribArray(1);
        gl3::VertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

        gl3::BindBuffer(GL_ARRAY_BUFFER, gpu.texcoords);
        gl3::BufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.texcoords.size() * sizeof(float)), mesh.texcoords.data(), GL_STATIC_DRAW);
        gl3::EnableVertexAttribArray(2);
        gl3::VertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        gl3::BindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.indices);
        gl3::BufferData(GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(uint32_t)),
            mesh.indices.data(), GL_STATIC_DRAW);

    }
}

void ModelPreview::RenderScene(int w, int h)
{
    gl3::EnsureLoaded();
    const ModelGLState savedState;
    EnsureProgram();
    EnsureFBO(w, h);
    if (gpuMeshes.empty()) UploadMeshes();
    if (!whiteTexture) whiteTexture = UploadTexture(std::vector<uint8_t>(4, 255), 1, 1);

    gl3::BindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, w, h);
    glClearColor(0.18f, 0.19f, 0.22f, 1.f);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    if (twoSided) glDisable(GL_CULL_FACE); else glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    float eyeX = centerX + distance * std::cos(pitch) * std::sin(yaw);
    float eyeY = centerY + distance * std::sin(pitch);
    float eyeZ = centerZ + distance * std::cos(pitch) * std::cos(yaw);

    float proj[16], view[16], model[16], mvp[16], tmp[16];
    Mat4Perspective(proj, 0.785398f, static_cast<float>(w) / static_cast<float>(h), 0.05f, distance * 20.f + 10.f);
    Mat4LookAt(view, eyeX, eyeY, eyeZ, centerX, centerY, centerZ, 0, 1, 0);
    Mat4Identity(model);
    Mat4Mul(tmp, view, model);
    Mat4Mul(mvp, proj, tmp);

    gl3::UseProgram(program);
    const int locMVP = gl3::GetUniformLocation(program, "uMVP");
    const int locModel = gl3::GetUniformLocation(program, "uModel");
    const int locLight = gl3::GetUniformLocation(program, "uLightDir");
    const int locView = gl3::GetUniformLocation(program, "uViewPos");
    const int locColor = gl3::GetUniformLocation(program, "uColor");
    gl3::UniformMatrix4fv(locMVP, 1, GL_FALSE, mvp);
    gl3::UniformMatrix4fv(locModel, 1, GL_FALSE, model);
    gl3::Uniform3f(locLight, 0.4f, -0.8f, 0.3f);
    gl3::Uniform3f(locView, eyeX, eyeY, eyeZ);
    gl3::Uniform1i(gl3::GetUniformLocation(program, "uTexture"), 0);

    static const float kColors[][3] = {
        {0.75f, 0.78f, 0.82f}, {0.85f, 0.55f, 0.45f}, {0.45f, 0.70f, 0.55f},
        {0.55f, 0.60f, 0.85f}, {0.80f, 0.70f, 0.40f}, {0.70f, 0.50f, 0.70f}
    };

    for (size_t mi = 0; mi < meshes.size(); ++mi)
    {
        const ModelMesh& mesh = meshes[mi];
        if (mesh.indices.empty()) continue;
        gl3::Uniform1f(gl3::GetUniformLocation(program, "uUVDivisor"), mesh.integerUV ? uvDivisor : 1.f);
        const unsigned int texture = textured && mesh.hasUV ? MaterialTextureID(mesh.material) : 0;
        glBindTexture(GL_TEXTURE_2D, texture ? texture : whiteTexture);
        gl3::Uniform1i(gl3::GetUniformLocation(program, "uTextured"), texture != 0);
        gl3::Uniform1i(gl3::GetUniformLocation(program, "uAlphaCutout"), mesh.material < materials.size() && materials[mesh.material].alphaCutout);
        const float* col = texture && mesh.material < materials.size() ? materials[mesh.material].color : kColors[mesh.material % 6];
        gl3::Uniform3f(locColor, col[0], col[1], col[2]);

        gl3::BindVertexArray(gpuMeshes[mi].vao);
        glDrawElements(GL_TRIANGLES, static_cast<int>(mesh.indices.size()), GL_UNSIGNED_INT, nullptr);
    }

}

void ModelPreview::Draw(PackageSession* session)
{
    ImGui::TextWrapped("%s", name.c_str());
    if (!error.empty())
    {
        ImGui::TextWrapped("Preview unavailable: %s", error.c_str());
        return;
    }
    ImGui::TextDisabled("%zu mesh%s, %zu verts, %zu tris",
        meshCount, meshCount == 1 ? "" : "es", vertexCount, triangleCount);
    if (ImGui::Checkbox("Package textures", &textured)) renderDirty = true;
    ImGui::SameLine();
    if (ImGui::Checkbox("Two-sided", &twoSided)) renderDirty = true;
    ImGui::SameLine();
    if (ImGui::Button("Frame model")) { ComputeBounds(); renderDirty = true; }
    if (!materials.empty() && ImGui::CollapsingHeader("Materials / replace textures")) {
        if (ImGui::BeginCombo("Material", materials[materialIndex].name.c_str())) {
            for (size_t i = 0; i < materials.size(); ++i)
                if (ImGui::Selectable(materials[i].name.c_str(), materialIndex == int(i))) materialIndex = int(i);
            ImGui::EndCombo();
        }
        auto& mat = materials[materialIndex];
        const auto& texture = materialTextures[materialIndex];
        if (mat.texture) ImGui::TextWrapped("Package texture: %s", mat.texture->GetName().c_str());
        else if (mat.textureCRC) ImGui::TextWrapped("Texture CRC 0x%08X is in another package. A PNG can be previewed here.", mat.textureCRC);
        else ImGui::TextWrapped("This material has no base-color texture.");
        if (ImGui::Checkbox("Cutout alpha", &mat.alphaCutout)) renderDirty = true;
        if (!mat.passes.empty() && ImGui::Button("Export material shaders...")) {
            try {
                const auto path = PickFile(glfwGetCurrentContext(), "Shader containers", "bin", true, SanitizeFileName(mat.name) + ".bin");
                if (!path.empty()) { ExportModelShaders(mat, path); textureStatus = "Shader containers and binding metadata exported for XenosRecomp."; }
            } catch (const std::exception& ex) { textureStatus = ex.what(); }
        }
        if (!texture.png.empty()) ImGui::TextWrapped("Preview: %s", texture.png.c_str());
        try {
            if (session && mat.texture && ImGui::Button("Extract package PNG...")) {
                const auto path = PickFile(glfwGetCurrentContext(), "PNG image", "png", true, SanitizeFileName(mat.texture->GetName()) + ".png");
                if (!path.empty()) { ExtractTexture(*session, *mat.texture, path); textureStatus = "Package texture extracted."; }
            }
            if (ImGui::Button("Preview PNG...")) {
                const auto path = PickFile(glfwGetCurrentContext(), "PNG image", "png", false);
                if (!path.empty()) LoadPNG(materialIndex, path);
            }
            ImGui::SameLine();
            if (ImGui::Button("Use package texture")) UsePackageTexture(materialIndex);
            if (session && mat.texture && !texture.png.empty()) {
                if (ImGui::Button("Apply PNG to package")) ApplyPNG(*session, materialIndex);
                ImGui::TextWrapped("Applies to every use of this texture. Save As writes the edited package.");
            }
        } catch (const std::exception& ex) { textureStatus = ex.what(); }
        if (!texture.error.empty()) ImGui::TextWrapped("%s", texture.error.c_str());
        if (!textureStatus.empty()) ImGui::TextWrapped("%s", textureStatus.c_str());
        if (ImGui::TreeNode("UV settings")) {
            if (ImGui::DragFloat("Integer UV divisor", &uvDivisor, 1, 1, 65536, "%.0f", ImGuiSliderFlags_AlwaysClamp)) renderDirty = true;
            ImGui::TextWrapped("1024 is confirmed in the OfficeSuit vertex shader. Animated UV transforms are not emulated.");
            ImGui::TreePop();
        }
    }
    ImGui::TextDisabled("Drag to orbit, scroll to zoom (static mesh preview)");

    const ImVec2 available = ImGui::GetContentRegionAvail();
    if (available.x < 32 || available.y < 32) return;

    const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
    const float pixelW = available.x * scale.x, pixelH = available.y * scale.y;
    const float fit = std::min({1.0f, 1920.f / pixelW, 1080.f / pixelH});
    const int rw = std::max(1, static_cast<int>(pixelW * fit));
    const int rh = std::max(1, static_cast<int>(pixelH * fit));

    ImGui::InvisibleButton("##modelview", available);
    const bool hovered = ImGui::IsItemHovered();
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        const ImVec2 delta = ImGui::GetIO().MouseDelta;
        renderDirty = renderDirty || delta.x != 0 || delta.y != 0;
        yaw += delta.x * 0.01f;
        pitch += delta.y * 0.01f;
        pitch = std::clamp(pitch, -1.4f, 1.4f);
    }
    if (hovered)
    {
        const float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.f) {
            distance = std::clamp(distance * (wheel > 0 ? 0.9f : 1.11f), 0.2f, 1000000.f);
            renderDirty = true;
        }
    }

    try
    {
        if (renderDirty || fboW != rw || fboH != rh) {
            RenderScene(rw, rh);
            renderDirty = false;
        }
    }
    catch (const std::exception& ex)
    {
        error = ex.what();
        ImGui::TextWrapped("Render error: %s", error.c_str());
        return;
    }

    const ImVec2 origin = ImGui::GetItemRectMin();
    ImGui::GetWindowDrawList()->AddImage(
        static_cast<ImTextureID>(colorTex),
        origin, ImVec2(origin.x + available.x, origin.y + available.y),
        ImVec2(0, 1), ImVec2(1, 0));
}
unsigned int ModelPreview::UploadTexture(const std::vector<uint8_t>& rgba, uint32_t width, uint32_t height)
{
    const size_t bytes = size_t(width) * height * 4;
    if (!width || !height || width > 4096 || height > 4096 || rgba.size() != bytes)
        throw std::runtime_error("Model textures must be between 1 and 4096 pixels per side.");
    if (bytes > 128 * 1024 * 1024 - textureBytes) throw std::runtime_error("Model textures exceed the 128 MiB preview budget.");
    gl3::EnsureLoaded();
    const ModelGLState state;
    GLint alignment, rowLength, skipRows, skipPixels, unpackBuffer;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
    glGetIntegerv(0x0CF2, &rowLength); glGetIntegerv(0x0CF3, &skipRows); glGetIntegerv(0x0CF4, &skipPixels);
    glGetIntegerv(0x88EF, &unpackBuffer);
    gl3::BindBuffer(0x88EC, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(0x0CF2, 0); glPixelStorei(0x0CF3, 0); glPixelStorei(0x0CF4, 0);
    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    GLint uploadedWidth = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &uploadedWidth);
    glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    glPixelStorei(0x0CF2, rowLength); glPixelStorei(0x0CF3, skipRows); glPixelStorei(0x0CF4, skipPixels);
    gl3::BindBuffer(0x88EC, unpackBuffer);
    if (uploadedWidth != static_cast<GLint>(width)) {
        if (texture) glDeleteTextures(1, &texture);
        throw std::runtime_error("Could not allocate model texture on the GPU.");
    }
    textureBytes += bytes;
    return texture;
}

unsigned int ModelPreview::MaterialTextureID(size_t index)
{
    if (index >= materials.size()) return 0;
    auto& cached = materialTextures[index];
    if (cached.attempted) return cached.texture;
    cached.attempted = true;
    if (!materials[index].texture || !package) return 0;
    try {
        const auto texture = RZTexture::Load(*package, *materials[index].texture);
        if (!texture.desc.Is2D() || texture.desc.width > 4096 || texture.desc.height > 4096)
            throw std::runtime_error("Unsupported or oversized material texture.");
        cached.texture = UploadTexture(texture.DecodeLevel0(), texture.desc.width, texture.desc.height);
        cached.bytes = size_t(texture.desc.width) * texture.desc.height * 4;
        glBindTexture(GL_TEXTURE_2D, cached.texture);
        const GLint address = materials[index].addressMode == 1 ? GL_CLAMP_TO_EDGE : GL_REPEAT;
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, address);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, address);
    } catch (const std::exception& ex) { cached.error = ex.what(); }
    return cached.texture;
}

void ModelPreview::LoadPNG(size_t index, const std::string& path)
{
    if (index >= materials.size()) throw std::runtime_error("Select a material first.");
    const auto image = BUPng::Decode(ReadFile(std::filesystem::u8path(path)));
    auto& cached = materialTextures[index];
    const auto texture = UploadTexture(image.rgba, image.width, image.height);
    if (cached.texture) glDeleteTextures(1, &cached.texture);
    textureBytes -= cached.bytes;
    cached = {texture, true, size_t(image.width) * image.height * 4, path, {}};
    textureStatus = "PNG preview only. The package has not been changed.";
    textured = true;
    renderDirty = true;
}

void ModelPreview::UsePackageTexture(size_t index)
{
    if (index >= materialTextures.size()) return;
    auto& cached = materialTextures[index];
    if (cached.texture) glDeleteTextures(1, &cached.texture);
    textureBytes -= cached.bytes;
    cached = {};
    textureStatus.clear();
    textured = true;
    renderDirty = true;
}

void ModelPreview::ApplyPNG(PackageSession& session, size_t index)
{
    if (session.package.get() != package || session.revision != revision || index >= materials.size() || !materials[index].texture)
        throw std::runtime_error("Select a material with a texture in this package first.");
    const auto& cached = materialTextures[index];
    if (cached.png.empty()) throw std::runtime_error("Preview a replacement PNG first.");
    const auto texture = RZTexture::Load(*package, *materials[index].texture);
    ReplaceTexture(session, *materials[index].texture, cached.png, texture.desc.width, texture.desc.height);
    textureStatus = session.status;
    renderDirty = true;
}
