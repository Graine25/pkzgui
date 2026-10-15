#include "shader/shader_program.h"
#include "platform/file_io.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace {
template<class T>T Proc(const char* name){auto f=reinterpret_cast<T>(glfwGetProcAddress(name));if(!f)throw std::runtime_error(std::string("Shader previews need OpenGL uniform buffers: ")+name);return f;}
struct Functions {
#define FN(ret,name,args) using name##Type=ret(*)args;name##Type name=Proc<name##Type>(#name)
    FN(unsigned,glCreateShader,(unsigned));FN(void,glShaderSource,(unsigned,int,const char* const*,const int*));
    FN(void,glCompileShader,(unsigned));FN(void,glGetShaderiv,(unsigned,unsigned,int*));FN(void,glGetShaderInfoLog,(unsigned,int,int*,char*));FN(void,glDeleteShader,(unsigned));
    FN(unsigned,glCreateProgram,());FN(void,glAttachShader,(unsigned,unsigned));FN(void,glBindAttribLocation,(unsigned,unsigned,const char*));
    FN(void,glLinkProgram,(unsigned));FN(void,glGetProgramiv,(unsigned,unsigned,int*));FN(void,glGetProgramInfoLog,(unsigned,int,int*,char*));FN(void,glDeleteProgram,(unsigned));
    FN(void,glUseProgram,(unsigned));FN(int,glGetUniformLocation,(unsigned,const char*));FN(void,glUniform1i,(int,int));
    FN(unsigned,glGetUniformBlockIndex,(unsigned,const char*));FN(void,glUniformBlockBinding,(unsigned,unsigned,unsigned));
    FN(void,glGenBuffers,(int,unsigned*));FN(void,glDeleteBuffers,(int,const unsigned*));FN(void,glBindBuffer,(unsigned,unsigned));FN(void,glBufferData,(unsigned,ptrdiff_t,const void*,unsigned));FN(void,glBindBufferBase,(unsigned,unsigned,unsigned));
    FN(void,glActiveTexture,(unsigned));FN(void,glGetIntegeri_v,(unsigned,unsigned,int*));
#undef FN
};
Functions& GL(){static Functions gl;return gl;}
unsigned Compile(unsigned type,const std::string& source)
{
    auto& g=GL();const unsigned shader=g.glCreateShader(type);const char* text=source.c_str();g.glShaderSource(shader,1,&text,nullptr);g.glCompileShader(shader);
    int success=0;g.glGetShaderiv(shader,0x8B81,&success);
    if(!success){char message[8192]{};g.glGetShaderInfoLog(shader,sizeof(message),nullptr,message);g.glDeleteShader(shader);throw std::runtime_error(std::string("Host shader compile: ")+message);}
    return shader;
}
void Defaults(ShaderConstants& data,const ShaderAsset& asset,const float* mvp,const float* view,const float* color,bool cameraOnly)
{
    for(const auto& c:asset.constants){
        auto& registers=c.pixel?data.pixel:data.vertex;
        const auto matrix=[&](const char* prefix,const float* values){
            const std::string p=prefix;if(c.name.size()!=p.size()+1 || c.name.compare(0,p.size(),p)!=0)return false;
            const int row=c.name.back()-'0';if(row<0||row>3)return false;
            for(int k=0;k<4;++k)registers[c.index*4+k]=values[k*4+row];return true;
        };
        if(!c.pixel && matrix("kVS_ViewProj",mvp))continue;
        if(!c.pixel && matrix("kVS_View",view))continue;
        if(c.pixel && matrix("kPS_View",view))continue;
        if(cameraOnly)continue;
        float* value=registers.data()+c.index*4;
        if(!c.pixel && c.name.rfind("kVS_World",0)==0){const char last=c.name.back();if(last>='0'&&last<='2')value[last-'0']=1;}
        if(c.pixel && c.name=="kPS_ColorFactor")std::copy_n(color,4,value);
        if(c.pixel && c.name=="kPS_AmbientLight"){value[0]=value[1]=value[2]=0.7f;value[3]=1;}
        if(c.pixel && c.name=="kPS_BumpFactor")std::fill_n(value,4,1.f);
    }
}
}
ShaderGLState::ShaderGLState()
{
    auto& g=GL();glGetIntegerv(0x84E0,&activeTexture);glGetIntegerv(0x8A28,&uniformBuffer);
    for(unsigned i=0;i<16;++i){g.glActiveTexture(0x84C0+i);glGetIntegerv(GL_TEXTURE_BINDING_2D,&textures[i]);}
    for(unsigned i=0;i<3;++i)g.glGetIntegeri_v(0x8A28,i,&buffers[i]);
    g.glActiveTexture(activeTexture);
}
ShaderGLState::~ShaderGLState()
{
    auto& g=GL();for(unsigned i=0;i<16;++i){g.glActiveTexture(0x84C0+i);glBindTexture(GL_TEXTURE_2D,textures[i]);}
    for(unsigned i=0;i<3;++i)g.glBindBufferBase(0x8A11,i,buffers[i]);
    g.glBindBuffer(0x8A11,uniformBuffer);g.glActiveTexture(activeTexture);
}
ShaderProgram::~ShaderProgram(){Clear();}
void ShaderProgram::Clear()
{
    if(program){auto& g=GL();g.glDeleteProgram(program);g.glDeleteBuffers(3,buffers);}
    program=0;std::fill_n(buffers,3,0);enabled=false;
}
void ShaderProgram::Load(const ShaderAsset& source)
{
    if(source.vertexGLSL.empty()||source.pixelGLSL.empty())throw std::runtime_error("Shader preview needs both stages.");
    auto& g=GL();const auto vs=Compile(0x8B31,source.vertexGLSL);unsigned ps=0,next=0;
    try{
        ps=Compile(0x8B30,source.pixelGLSL);next=g.glCreateProgram();g.glAttachShader(next,vs);g.glAttachShader(next,ps);
        const char* names[]={"gsxPosition","gsxNormal","gsxUV","gsxTangent","gsxColor"};
        for(unsigned i=0;i<5;++i)g.glBindAttribLocation(next,i,names[i]);
        g.glLinkProgram(next);int success=0;g.glGetProgramiv(next,0x8B82,&success);
        if(!success){char message[8192]{};g.glGetProgramInfoLog(next,sizeof(message),nullptr,message);throw std::runtime_error(std::string("Shader stages do not link: ")+message);}
    }catch(...){if(next)g.glDeleteProgram(next);if(ps)g.glDeleteShader(ps);g.glDeleteShader(vs);throw;}
    g.glDeleteShader(vs);g.glDeleteShader(ps);
    Clear();program=next;asset=source;enabled=true;captured=false;capturedCamera=false;g.glGenBuffers(3,buffers);
    const char* blocks[]={"type_GsxVertexBlock","type_GsxPixelBlock","type_GsxSharedBlock"};
    for(unsigned i=0;i<3;++i){const auto block=g.glGetUniformBlockIndex(program,blocks[i]);if(block!=0xFFFFFFFFu)g.glUniformBlockBinding(program,block,i);}
    int previous=0;glGetIntegerv(0x8B8D,&previous);g.glUseProgram(program);
    for(int i=0;i<16;++i)g.glUniform1i(g.glGetUniformLocation(program,("gsxTextures["+std::to_string(i)+"]").c_str()),i);
    g.glUseProgram(previous);
}
void ShaderProgram::LoadConstants(const std::string& path)
{
    const auto file=std::filesystem::u8path(path);
    if(std::filesystem::file_size(file)!=sizeof(ShaderConstants))
        throw std::runtime_error("Expected a 9216-byte game draw constant snapshot.");
    const auto bytes=ReadFile(file);
    if(bytes.size()!=sizeof(ShaderConstants))throw std::runtime_error("Expected 9216 bytes: little-endian VS float4[256], PS float4[256], shared uint4[64].");
    ShaderConstants next;std::memcpy(&next,bytes.data(),bytes.size());
    for(const auto* values:{&next.vertex,&next.pixel})for(float value:*values)if(!std::isfinite(value))throw std::runtime_error("Captured constants contain non-finite values.");
    capturedConstants=next;captured=true;
}
void ShaderProgram::Bind(const float* mvp,const float* view,const float* color,bool alphaCutout,uint32_t signs,uint32_t alphaSigns)
{
    auto& g=GL();ShaderConstants data=captured?capturedConstants:ShaderConstants{};
    if(!captured || !capturedCamera)Defaults(data,asset,mvp,view,color,captured);
    // Vertex attributes are already unpacked and endian-correct on the CPU.
    for(int i:{88,92,93,94,95,96})data.shared[i]=0;
    data.shared[97]&=0xFFFF0000u;
    data.shared[99]&=0x80000000u;
    data.shared[252]=captured?(data.shared[252]&2u):(alphaCutout?2u:0u);
    if(!captured){for(int i=228;i<232;++i)data.shared[i]=0x3f800000u;data.shared[91]=0x3e99999a;data.shared[97]=alphaSigns;data.shared[98]=signs;data.shared[99]=0x80000000u;}
    g.glUseProgram(program);
    const void* sources[]={data.vertex.data(),data.pixel.data(),data.shared.data()};
    const size_t sizes[]={sizeof(data.vertex),sizeof(data.pixel),sizeof(data.shared)};
    for(unsigned i=0;i<3;++i){g.glBindBuffer(0x8A11,buffers[i]);g.glBufferData(0x8A11,sizes[i],sources[i],0x88E8);g.glBindBufferBase(0x8A11,i,buffers[i]);}
}
