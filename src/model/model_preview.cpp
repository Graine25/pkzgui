#include "model/model_preview.h"
#include "package/resource_selection.h"
#include "imgui.h"
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
        GLint depthFunc, cullMode, frontFace;
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
uniform mat4 uMVP;
uniform mat4 uModel;
out vec3 vNrm;
out vec3 vWorld;
void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorld = world.xyz;
    vNrm = mat3(uModel) * aNrm;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

static const char* kFragmentShader = R"(#version 150
in vec3 vNrm;
in vec3 vWorld;
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
    vec3 ambient = 0.25 * uColor;
    vec3 diffuse = 0.65 * diff * uColor;
    vec3 specular = 0.25 * spec * vec3(1.0);
    fragColor = vec4(ambient + diffuse + specular, 1.0);
}
)";
#else
static const char* kVertexShader = R"(#version 130
in vec3 aPos;
in vec3 aNrm;
uniform mat4 uMVP;
uniform mat4 uModel;
out vec3 vNrm;
out vec3 vWorld;
void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorld = world.xyz;
    vNrm = mat3(uModel) * aNrm;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

static const char* kFragmentShader = R"(#version 130
in vec3 vNrm;
in vec3 vWorld;
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
    vec3 ambient = 0.25 * uColor;
    vec3 diffuse = 0.65 * diff * uColor;
    vec3 specular = 0.25 * spec * vec3(1.0);
    fragColor = vec4(ambient + diffuse + specular, 1.0);
}
)";
#endif

namespace {

    uint16_t ReadBE16(const uint8_t* p)
    {
        return static_cast<uint16_t>((p[0] << 8) | p[1]);
    }

    uint32_t ReadBE32(const uint8_t* p)
    {
        return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
            (static_cast<uint32_t>(p[2]) << 8) | p[3];
    }

    float ReadBEf32(const uint8_t* p)
    {
        uint32_t u = ReadBE32(p);
        float f;
        std::memcpy(&f, &u, 4);
        return f;
    }

    int s11(int v) { return v > 1023 ? v - 2048 : v; }
    int s10(int v) { return v > 511 ? v - 1024 : v; }

    void UnpackNormal(uint32_t u, float& x, float& y, float& z)
    {
        x = s11(static_cast<int>(u & 0x7FF)) / 1023.0f;
        y = s11(static_cast<int>((u >> 11) & 0x7FF)) / 1023.0f;
        z = s10(static_cast<int>((u >> 22) & 0x3FF)) / 511.0f;
    }

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


    struct Desc
    {
        uint32_t mat = 0, vboff = 0, stride = 0, fmt = 0, nv = 0, idxoff = 0, nidx = 0;
    };

    int Nuv(uint32_t fmt) { return (fmt >> 5) & 7; }

    int CalcStride(uint32_t fmt)
    {
        int s = 12;
        if (fmt & 0x0002) s += 4;
        if (fmt & 0x0004) s += 4;
        if (fmt & 0x0010) s += 4;
        s += 4 * Nuv(fmt);
        if (fmt & 0x1000) s += 8;
        return s;
    }

    struct Offsets
    {
        int nrm = -1, tan = -1, col = -1, uv = -1, skin = -1;
    };

    Offsets AttrOffsets(uint32_t fmt)
    {
        Offsets o;
        int p = 12;
        if (fmt & 0x0002) { o.nrm = p; p += 4; }
        if (fmt & 0x0004) { o.tan = p; p += 4; }
        if (fmt & 0x0010) { o.col = p; p += 4; }
        if (Nuv(fmt)) { o.uv = p; p += 4 * Nuv(fmt); }
        if (fmt & 0x1000) o.skin = p;
        return o;
    }

    std::vector<uint32_t> StripToTris(const uint16_t* idx, size_t n, uint32_t nv)
    {
        std::vector<uint32_t> tris;
        std::vector<uint16_t> run;
        auto flush = [&]() {
            for (size_t i = 0; i + 2 < run.size(); ++i)
            {
                const uint16_t a = run[i], b = run[i + 1], c = run[i + 2];
                if (a == b || b == c || a == c) continue;
                if (a >= nv || b >= nv || c >= nv) continue;
                if (i % 2 == 0) { tris.push_back(b); tris.push_back(a); tris.push_back(c); }
                else { tris.push_back(a); tris.push_back(b); tris.push_back(c); }
            }
            run.clear();
            };
        for (size_t i = 0; i < n; ++i)
        {
            const uint16_t x = idx[i];
            if (x == 0xFFFF || x == 0xAAAA || x >= nv)
            {
                flush();
                continue;
            }
            run.push_back(x);
        }
        flush();
        return tris;
    }

    std::vector<uint32_t> ListToTris(const uint16_t* idx, size_t n, uint32_t nv)
    {
        std::vector<uint32_t> tris;
        for (size_t i = 0; i + 2 < n; i += 3)
        {
            const uint16_t a = idx[i], b = idx[i + 1], c = idx[i + 2];
            if (a == 0xFFFF || b == 0xFFFF || c == 0xFFFF) continue;
            if (a == 0xAAAA || b == 0xAAAA || c == 0xAAAA) continue;
            if (a >= nv || b >= nv || c >= nv) continue;
            if (a == b || b == c || a == c) continue;
            tris.push_back(a);
            tris.push_back(b);
            tris.push_back(c);
        }
        return tris;
    }

    float TopologyScore(const std::vector<float>& pos, const std::vector<uint32_t>& tris)
    {
        if (tris.size() < 3 || pos.size() < 9) return 1e30f;
        std::vector<float> edges;
        edges.reserve(tris.size());
        float maxLen = 0;
        auto elen = [&](uint32_t ia, uint32_t ib) {
            const float dx = pos[ia * 3] - pos[ib * 3];
            const float dy = pos[ia * 3 + 1] - pos[ib * 3 + 1];
            const float dz = pos[ia * 3 + 2] - pos[ib * 3 + 2];
            return std::sqrt(dx * dx + dy * dy + dz * dz);
            };
        for (size_t i = 0; i + 2 < tris.size(); i += 3)
        {
            const float e0 = elen(tris[i], tris[i + 1]);
            const float e1 = elen(tris[i + 1], tris[i + 2]);
            const float e2 = elen(tris[i + 2], tris[i]);
            edges.push_back(e0); edges.push_back(e1); edges.push_back(e2);
            maxLen = std::max(maxLen, std::max(e0, std::max(e1, e2)));
        }
        if (edges.empty()) return 1e30f;
        std::nth_element(edges.begin(), edges.begin() + static_cast<std::ptrdiff_t>(edges.size() / 2), edges.end());
        const float median = edges[edges.size() / 2];
        if (median < 1e-8f) return 1e30f;
        const float outlier = maxLen / median;
        const float density = static_cast<float>(tris.size() / 3);
        return outlier * 10.f - std::log(density + 1.f);
    }

    std::vector<uint32_t> FilterLongEdges(const std::vector<float>& pos, const std::vector<uint32_t>& tris,
        float maxFactor = 10.f)
    {
        if (tris.size() < 3) return tris;
        auto elen = [&](uint32_t ia, uint32_t ib) {
            const float dx = pos[ia * 3] - pos[ib * 3];
            const float dy = pos[ia * 3 + 1] - pos[ib * 3 + 1];
            const float dz = pos[ia * 3 + 2] - pos[ib * 3 + 2];
            return std::sqrt(dx * dx + dy * dy + dz * dz);
            };
        std::vector<float> edges;
        edges.reserve(tris.size());
        for (size_t i = 0; i + 2 < tris.size(); i += 3)
        {
            edges.push_back(elen(tris[i], tris[i + 1]));
            edges.push_back(elen(tris[i + 1], tris[i + 2]));
            edges.push_back(elen(tris[i + 2], tris[i]));
        }
        if (edges.empty()) return tris;
        std::nth_element(edges.begin(), edges.begin() + static_cast<std::ptrdiff_t>(edges.size() / 2), edges.end());
        const float median = edges[edges.size() / 2];
        const float limit = std::max(median * maxFactor, median + 1e-4f);
        std::vector<uint32_t> out;
        out.reserve(tris.size());
        for (size_t i = 0; i + 2 < tris.size(); i += 3)
        {
            const float e0 = elen(tris[i], tris[i + 1]);
            const float e1 = elen(tris[i + 1], tris[i + 2]);
            const float e2 = elen(tris[i + 2], tris[i]);
            if (e0 > limit || e1 > limit || e2 > limit) continue;
            out.push_back(tris[i]);
            out.push_back(tris[i + 1]);
            out.push_back(tris[i + 2]);
        }
        return out;
    }

    bool ParseDescriptor(const uint8_t* data, size_t size, Desc& d)
    {
        if (size < 48) return false;
        d.mat = ReadBE32(data + 0);
        d.vboff = ReadBE32(data + 4);
        d.stride = ReadBE32(data + 8);
        d.fmt = ReadBE32(data + 12);
        d.nv = ReadBE32(data + 16);
        d.idxoff = ReadBE32(data + 20);
        d.nidx = ReadBE32(data + 24);
        if (d.nv == 0 || d.nidx == 0 || d.stride == 0) return false;
        (void)CalcStride;
        return true;
    }

    bool FinitePos(float x, float y, float z)
    {
        if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return false;
        constexpr float lim = 1.0e5f;
        return std::fabs(x) < lim && std::fabs(y) < lim && std::fabs(z) < lim;
    }

    void DecodeMesh(const uint8_t* vb, size_t vbSize, const uint8_t* ib, size_t ibSize,
        const Desc& d, ModelMesh& out)
    {
        out.material = d.mat;
        out.positions.clear();
        out.normals.clear();
        out.indices.clear();

        const uint32_t stride = d.stride;
        if (stride < static_cast<uint32_t>(CalcStride(d.fmt)) || d.nv < 3 || d.nidx < 3) return;
        if (d.nv > 1000000 || d.nidx > 6000000)
            throw std::runtime_error("Mesh exceeds the preview limit (1M vertices / 6M indices).");
        if (d.vboff + static_cast<size_t>(d.nv) * stride > vbSize) return;
        if (static_cast<size_t>(d.idxoff) * 2 + static_cast<size_t>(d.nidx) * 2 > ibSize) return;

        const Offsets o = AttrOffsets(d.fmt);
        out.positions.reserve(static_cast<size_t>(d.nv) * 3);
        out.normals.reserve(static_cast<size_t>(d.nv) * 3);

        int bad = 0;
        for (uint32_t v = 0; v < d.nv; ++v)
        {
            const uint8_t* p = vb + d.vboff + static_cast<size_t>(v) * stride;
            const float x = ReadBEf32(p + 0);
            const float y = ReadBEf32(p + 4);
            const float z = ReadBEf32(p + 8);
            if (!FinitePos(x, y, z)) ++bad;
            out.positions.push_back(x);
            out.positions.push_back(y);
            out.positions.push_back(z);
            if (o.nrm >= 0)
            {
                float nx, ny, nz;
                UnpackNormal(ReadBE32(p + o.nrm), nx, ny, nz);
                out.normals.push_back(nx);
                out.normals.push_back(ny);
                out.normals.push_back(nz);
            }
            else
            {
                out.normals.push_back(0.f);
                out.normals.push_back(1.f);
                out.normals.push_back(0.f);
            }
        }
        if (bad != 0)
        {
            out.positions.clear();
            out.normals.clear();
            return;
        }

        uint32_t nidx = d.nidx;
        while (nidx > 0)
        {
            const uint16_t last = ReadBE16(ib + (static_cast<size_t>(d.idxoff) + nidx - 1) * 2);
            if (last != 0xAAAA) break;
            --nidx;
        }
        if (nidx < 3) return;

        std::vector<uint16_t> raw(nidx);
        for (uint32_t i = 0; i < nidx; ++i)
            raw[i] = ReadBE16(ib + (static_cast<size_t>(d.idxoff) + i) * 2);

        auto strip = StripToTris(raw.data(), raw.size(), d.nv);
        auto list = ListToTris(raw.data(), raw.size(), d.nv);
        const float stripScore = TopologyScore(out.positions, strip);
        const float listScore = TopologyScore(out.positions, list);
        out.indices = (listScore + 0.5f < stripScore) ? std::move(list) : std::move(strip);
        out.indices = FilterLongEdges(out.positions, out.indices, 10.f);
    }

    std::string Lower(std::string s)
    {
        for (char& c : s) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + 32);
        return s;
    }

    std::string NameFromHeader(const CMChunk& chunk)
    {
        if (chunk.GetMaskedID() != GenSub_ResourceHeader) return {};
        return CMChunkResourceHeader(chunk).GetName();
    }

    const CMChunk* FindChildId(const CMChunk& parent, uint32_t id)
    {
        for (const CMChunk& c : parent.children)
            if (c.GetMaskedID() == id)
                return &c;
        return nullptr;
    }

    struct ModelAsset
    {
        std::string name;
        const CMChunk* lod = nullptr;
        const uint8_t* vb = nullptr;
        size_t vbSize = 0;
        const uint8_t* ib = nullptr;
        size_t ibSize = 0;
    };

    void CollectModelAssets(const CMChunk& chunk, std::vector<ModelAsset>& out)
    {
        if (chunk.GetMaskedID() == 0x26 || chunk.GetMaskedID() == 0x0026)
        {
            std::string nm;
            const CMChunk* lod = nullptr;
            for (const CMChunk& c : chunk.children)
            {
                if (c.GetMaskedID() == 0x138E && nm.empty())
                    nm = NameFromHeader(c);
                if ((c.GetMaskedID() == 0x0325 || c.GetMaskedID() == 0x325) && !lod)
                    lod = &c;
            }
            if (lod)
            {
                const CMChunk* vb = nullptr;
                const CMChunk* ib = nullptr;
                for (const CMChunk& c : lod->children)
                {
                    if (c.GetMaskedID() == 0x1388) vb = &c;
                    if (c.GetMaskedID() == 0x1389) ib = &c;
                }
                if (vb && ib && !vb->data.empty() && !ib->data.empty())
                {
                    ModelAsset a;
                    a.name = nm;
                    a.lod = lod;
                    a.vb = vb->data.data();
                    a.vbSize = vb->data.size();
                    a.ib = ib->data.data();
                    a.ibSize = ib->data.size();
                    out.push_back(std::move(a));
                }
            }
        }
        for (const CMChunk& c : chunk.children)
            CollectModelAssets(c, out);
    }

    void CollectLodBuffers(const CMChunk& chunk, std::vector<ModelAsset>& out, const std::string& nearbyName)
    {
        std::string nm = nearbyName;
        if (const auto* header = chunk.FindChild(GenSub_ResourceHeader))
            nm = NameFromHeader(*header);
        if (chunk.GetMaskedID() == 0x138E)
        {
            std::string h = NameFromHeader(chunk);
            if (!h.empty()) nm = h;
        }

        if (chunk.GetMaskedID() == 0x0325 || chunk.GetMaskedID() == 0x325)
        {
            const CMChunk* vb = nullptr;
            const CMChunk* ib = nullptr;
            for (const CMChunk& c : chunk.children)
            {
                if (c.GetMaskedID() == 0x1388) vb = &c;
                if (c.GetMaskedID() == 0x1389) ib = &c;
            }
            if (vb && ib && !vb->data.empty() && !ib->data.empty())
            {
                ModelAsset a;
                a.name = nm;
                a.lod = &chunk;
                a.vb = vb->data.data();
                a.vbSize = vb->data.size();
                a.ib = ib->data.data();
                a.ibSize = ib->data.size();
                bool dup = false;
                for (const auto& e : out)
                    if (e.vb == a.vb && e.ib == a.ib) { dup = true; break; }
                if (!dup) out.push_back(std::move(a));
            }
        }

        for (const CMChunk& c : chunk.children)
            CollectLodBuffers(c, out, nm);
    }

    struct DescGroup
    {
        std::string name;
        const CMChunk* lod = nullptr;
        std::vector<Desc> descs;
    };

    void CollectDescriptors(const CMChunk& chunk, std::vector<DescGroup>& groups, const std::string& nearbyName)
    {
        std::string nm = nearbyName;
        if (const auto* header = chunk.FindChild(GenSub_ResourceHeader))
            nm = NameFromHeader(*header);
        if (chunk.GetMaskedID() == 0x138E)
        {
            std::string h = NameFromHeader(chunk);
            if (!h.empty()) nm = h;
        }

        if (chunk.GetMaskedID() == 0x0325 || chunk.GetMaskedID() == 0x325)
        {
            std::vector<Desc> local;
            for (const CMChunk& c : chunk.children)
            {
                if (c.GetMaskedID() == 0x0327)
                {
                    Desc d;
                    if (ParseDescriptor(c.data.data(), c.data.size(), d))
                        local.push_back(d);
                }
            }
            if (!local.empty())
            {
                DescGroup g;
                g.name = nm;
                g.lod = &chunk;
                g.descs = std::move(local);
                groups.push_back(std::move(g));
            }
            return;
        }

        if (chunk.GetMaskedID() == 0x0327)
        {
            Desc d;
            if (ParseDescriptor(chunk.data.data(), chunk.data.size(), d))
            {
                DescGroup g;
                g.name = nm;
                g.descs.push_back(d);
                groups.push_back(std::move(g));
            }
        }

        for (const CMChunk& c : chunk.children)
            CollectDescriptors(c, groups, nm);
    }

    int ScoreDescGroup(const ModelAsset& asset, const std::vector<Desc>& descs)
    {
        int score = 0;
        int ok = 0;
        for (const Desc& d : descs)
        {
            const uint32_t stride = d.stride;
            if (stride < static_cast<uint32_t>(CalcStride(d.fmt)) || d.nv < 3 || d.nidx < 3) continue;
            if (d.nv > 1000000 || d.nidx > 6000000) continue;
            if (d.vboff + static_cast<size_t>(d.nv) * stride > asset.vbSize) continue;
            if (static_cast<size_t>(d.idxoff) * 2 + static_cast<size_t>(d.nidx) * 2 > asset.ibSize) continue;
            int sampleBad = 0;
            const uint32_t samples = std::min(d.nv, 8u);
            for (uint32_t v = 0; v < samples; ++v)
            {
                const uint8_t* p = asset.vb + d.vboff + static_cast<size_t>(v) * stride;
                if (!FinitePos(ReadBEf32(p), ReadBEf32(p + 4), ReadBEf32(p + 8)))
                    ++sampleBad;
            }
            if (sampleBad * 2 >= static_cast<int>(samples)) continue;
            score += static_cast<int>(d.nv);
            ++ok;
        }
        return ok ? score : -1;
    }

    bool BuildMeshes(const ModelAsset& asset, const std::vector<Desc>& descs,
        std::vector<ModelMesh>& meshes)
    {
        bool any = false;
        for (const Desc& d : descs)
        {
            ModelMesh m;
            DecodeMesh(asset.vb, asset.vbSize, asset.ib, asset.ibSize, d, m);
            if (!m.indices.empty() && !m.positions.empty())
            {
                size_t bytes = (m.positions.size() + m.normals.size()) * sizeof(float) + m.indices.size() * sizeof(uint32_t);
                for (const auto& mesh : meshes)
                    bytes += (mesh.positions.size() + mesh.normals.size()) * sizeof(float) + mesh.indices.size() * sizeof(uint32_t);
                if (bytes > 128 * 1024 * 1024)
                    throw std::runtime_error("Model exceeds the 128 MiB preview budget.");
                meshes.push_back(std::move(m));
                any = true;
            }
        }
        return any;
    }

    struct ScanStats
    {
        int assets = 0, descGroups = 0, descs = 0, vb = 0, ib = 0, meshChunks = 0;
    };

    void CountGeo(const CMChunk& chunk, ScanStats& s)
    {
        const uint32_t id = chunk.GetMaskedID();
        if (id == 0x26 || id == 0x0026) ++s.assets;
        if (id == 0x1388) ++s.vb;
        if (id == 0x1389) ++s.ib;
        if (id == 0x0327) ++s.meshChunks;
        for (const CMChunk& c : chunk.children)
            CountGeo(c, s);
    }

    bool SearchPackageModels(const PKPackage& package, const std::string& wantName,
        std::vector<ModelMesh>& meshes, std::string& outName,
        std::string& diag)
    {
        std::vector<ModelAsset> assets;
        std::vector<DescGroup> groups;
        ScanStats stats;

        for (const CMChunk& root : package.rootChunks)
        {
            CollectModelAssets(root, assets);
            CollectLodBuffers(root, assets, "");
            CollectDescriptors(root, groups, "");
            CountGeo(root, stats);
        }
        stats.assets = static_cast<int>(assets.size());
        stats.descGroups = static_cast<int>(groups.size());
        for (const auto& g : groups) stats.descs += static_cast<int>(g.descs.size());

        diag = "scan: assets=" + std::to_string(stats.assets) +
            " vb=" + std::to_string(stats.vb) +
            " ib=" + std::to_string(stats.ib) +
            " geomMesh=" + std::to_string(stats.meshChunks) +
            " descGroups=" + std::to_string(stats.descGroups) +
            " descs=" + std::to_string(stats.descs);

        if (assets.empty())
            return false;

        const std::string want = Lower(wantName);

        auto tryAsset = [&](const ModelAsset& asset) -> bool {
            const std::string assetKey = Lower(asset.name);
            int bestScore = -1;
            const DescGroup* best = nullptr;

            for (const auto& g : groups)
            {
                const std::string gKey = Lower(g.name);
                if (g.lod != asset.lod && (assetKey.empty() || assetKey != gKey))
                    continue;
                if (!want.empty())
                {
                    if (assetKey != want && gKey != want && !assetKey.empty() && !gKey.empty())
                        continue;
                }
                const int s = ScoreDescGroup(asset, g.descs);
                if (s > bestScore)
                {
                    bestScore = s;
                    best = &g;
                }
            }

            if (!best || bestScore < 0) return false;
            if (!BuildMeshes(asset, best->descs, meshes)) return false;
            outName = !asset.name.empty() ? asset.name : best->name;
            if (outName.empty()) outName = "(unnamed)";
            return true;
            };

        if (!want.empty())
        {
            for (const auto& a : assets)
                if (!a.name.empty() && Lower(a.name) == want && tryAsset(a))
                    return true;
        }

        if (!want.empty()) return false;
        for (const auto& a : assets)
            if (!a.name.empty() && tryAsset(a))
                return true;
        for (const auto& a : assets)
            if (a.name.empty() && tryAsset(a))
                return true;

        return false;
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
        const unsigned int buffers[] = {mesh.positions, mesh.normals, mesh.indices};
        gl3::DeleteBuffers(3, buffers);
    }
    gpuMeshes.clear();
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

    std::string diag;
    const bool ok = SearchPackageModels(*session.package, want, meshes, name, diag);
    if (!ok || meshes.empty())
    {
        active = true;
        error = "No drawable mesh found. " + diag;
        if (resource)
            error += " resource=\"" + resource->GetName() + "\" type=" +
            std::to_string(resource->GetResourceType());
        else if (session.selected)
        {
            char b[16];
            std::snprintf(b, sizeof(b), "%X", session.selected->GetMaskedID());
            error += " chunk=0x";
            error += b;
        }
        return false;
    }

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
        if (DecodeGeometry(session))
            active = true;
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
    glEnable(GL_CULL_FACE);
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

    static const float kColors[][3] = {
        {0.75f, 0.78f, 0.82f}, {0.85f, 0.55f, 0.45f}, {0.45f, 0.70f, 0.55f},
        {0.55f, 0.60f, 0.85f}, {0.80f, 0.70f, 0.40f}, {0.70f, 0.50f, 0.70f}
    };

    for (size_t mi = 0; mi < meshes.size(); ++mi)
    {
        const ModelMesh& mesh = meshes[mi];
        if (mesh.indices.empty()) continue;
        const float* col = kColors[mi % 6];
        gl3::Uniform3f(locColor, col[0], col[1], col[2]);

        gl3::BindVertexArray(gpuMeshes[mi].vao);
        glDrawElements(GL_TRIANGLES, static_cast<int>(mesh.indices.size()), GL_UNSIGNED_INT, nullptr);
    }

}

void ModelPreview::Draw()
{
    ImGui::TextWrapped("%s", name.c_str());
    if (!error.empty())
    {
        ImGui::TextWrapped("Preview unavailable: %s", error.c_str());
        return;
    }
    ImGui::TextDisabled("%zu mesh%s, %zu verts, %zu tris",
        meshCount, meshCount == 1 ? "" : "es", vertexCount, triangleCount);
    ImGui::TextDisabled("Drag to orbit, scroll to zoom");

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