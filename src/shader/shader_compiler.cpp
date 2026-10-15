#include "pch.h"
#include "shader_recompiler.h"
#include "shader/shader_asset.h"
#include "platform/file_io.h"
#include "gsx_shader_common.h"
#include "spirv_glsl.hpp"
#include <mutex>
#include <set>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace {
std::mutex compilerMutex;
std::string Adapt(const std::string& source)
{
    std::string result = R"(cbuffer GsxVertexBlock : register(b0, space7) { float4 gsxVS[256]; };
cbuffer GsxPixelBlock : register(b1, space7) { float4 gsxPS[256]; };
cbuffer GsxSharedBlock : register(b2, space7) { uint4 gsxShared[64]; };
uint gsxSharedU(uint offset) { return gsxShared[offset / 16][(offset / 4) % 4]; }
uint4 gsxSharedU4(uint offset) { return gsxShared[offset / 16]; }
)";
    static const std::regex load(R"((\s*#define .*?)vk::RawBufferLoad<(\w+)>\(g_PushConstants\.(\w+) \+ )");
    static const std::regex alignment(R"(,\s*0x10$)");
    static const std::regex heap(R"(DescriptorHeap\[\])");
    static const std::regex sampler(R"(g_SamplerDescriptorHeap\[[^\]]+\])");
    static const std::regex integer("1ull");
    std::istringstream lines(source);std::string line;
    while(std::getline(lines,line)){
        std::smatch match;
        if(std::regex_search(line,match,load)){
            const std::string prefix=match[1],type=match[2],buffer=match[3];
            const size_t start=match.position()+match.length();
            size_t end=start;
            int depth=1;
            for(;end<line.size();++end){
                if(line[end]=='(')++depth;
                else if(line[end]==')' && --depth==0)break;
            }
            if(depth)throw std::runtime_error("Unbalanced shader constant load.");
            const std::string offset=std::regex_replace(line.substr(start,end-start),alignment,"");
            std::string value;
            if(prefix.find("DescriptorIndex")!=std::string::npos){
                size_t used=0;const auto n=std::stoul(offset,&used);if(used!=offset.size())throw std::runtime_error("Dynamic texture descriptor is unsupported.");value=std::to_string((n%64)/4);
            }else if(buffer=="VertexShaderConstants")value="gsxVS[("+offset+") / 16]";
            else if(buffer=="PixelShaderConstants")value="gsxPS[("+offset+") / 16]";
            else if(type=="uint")value="gsxSharedU("+offset+")";
            else if(type=="uint4")value="gsxSharedU4("+offset+")";
            else if(type=="float")value="asfloat(gsxSharedU("+offset+"))";
            else if(type=="float4")value="asfloat(gsxSharedU4("+offset+"))";
            else if(type=="float2")value="asfloat(uint2(gsxSharedU("+offset+"), gsxSharedU(("+offset+")+4)))";
            else throw std::runtime_error("Unsupported shared-constant type: "+type);
            line=prefix+value+line.substr(end+1);
        }
        line=std::regex_replace(line,heap,"DescriptorHeap[16]");
        line=std::regex_replace(line,sampler,"gsxSampler");
        line=std::regex_replace(line,integer,"1u");
        if(line=="[[vk::constant_id(0)]] const uint g_SpecConstants = 0;")line.clear();
        if(line=="#define g_SpecConstants() g_SpecConstants")line="#define g_SpecConstants() gsxSharedU(1008)";
        result+=line+'\n';
    }
    return result;
}
std::vector<uint32_t> SPIRV(const std::string& source,bool pixel)
{
    IDxcCompiler3* compiler=nullptr;
    if(FAILED(DxcCreateInstance(CLSID_DxcCompiler,IID_PPV_ARGS(&compiler))))throw std::runtime_error("Cannot create DXC compiler.");
    DxcBuffer input{};input.Ptr=source.data();input.Size=source.size();input.Encoding=DXC_CP_UTF8;
    const wchar_t* args[]={L"-E",L"shaderMain",L"-T",pixel?L"ps_6_0":L"vs_6_0",L"-HV",L"2021",L"-DREEOT_RECOMP",L"-spirv",L"-fvk-use-dx-layout"};
    IDxcResult* result=nullptr;const auto hr=compiler->Compile(&input,args,sizeof(args)/sizeof(args[0]),nullptr,IID_PPV_ARGS(&result));compiler->Release();
    if(FAILED(hr)||!result)throw std::runtime_error("DXC compilation failed.");
    HRESULT status=E_FAIL;result->GetStatus(&status);
    if(FAILED(status)){
        IDxcBlobUtf8* errors=nullptr;result->GetOutput(DXC_OUT_ERRORS,IID_PPV_ARGS(&errors),nullptr);
        const std::string message=errors?errors->GetStringPointer():"DXC compilation failed.";
        if(errors)errors->Release();result->Release();throw std::runtime_error(message);
    }
    IDxcBlob* blob=nullptr;result->GetOutput(DXC_OUT_OBJECT,IID_PPV_ARGS(&blob),nullptr);result->Release();
    if(!blob || blob->GetBufferSize()%4){if(blob)blob->Release();throw std::runtime_error("Invalid DXC SPIR-V output.");}
    const auto* first=static_cast<const uint32_t*>(blob->GetBufferPointer());std::vector<uint32_t> words(first,first+blob->GetBufferSize()/4);blob->Release();return words;
}
std::string GLSL(const std::string& hlsl,bool pixel)
{
    if(hlsl.size()>4*1024*1024)throw std::runtime_error("Shader source exceeds 4 MiB.");
    spirv_cross::CompilerGLSL c(SPIRV(Adapt(hlsl),pixel));
    auto options=c.get_common_options();options.version=150;options.es=false;options.enable_420pack_extension=false;options.vertex.fixup_clipspace=!pixel;c.set_common_options(options);
    auto resources=c.get_shader_resources();
    for(const auto& r:resources.stage_inputs){
        const auto loc=c.get_decoration(r.id,spv::DecorationLocation);
        if(pixel)c.set_name(r.id,"gsxVarying"+std::to_string(loc));
        else {
            const auto name=r.name;
            const char* target=nullptr;
            for(const auto& pair:std::vector<std::pair<std::string,const char*>>{{"POSITION0","gsxPosition"},{"NORMAL0","gsxNormal"},{"TANGENT0","gsxTangent"},{"COLOR0","gsxColor"},{"TEXCOORD0","gsxUV"},{"TEXCOORD1","gsxUV1"},{"TEXCOORD2","gsxUV2"},{"TEXCOORD3","gsxUV3"}})
                if(name.size()>=pair.first.size()&&name.compare(name.size()-pair.first.size(),pair.first.size(),pair.first)==0)target=pair.second;
            if(!target)throw std::runtime_error("Static shader preview does not support vertex input "+name);
            c.set_name(r.id,target);
        }
    }
    if(!pixel)for(const auto& r:resources.stage_outputs)c.set_name(r.id,"gsxVarying"+std::to_string(c.get_decoration(r.id,spv::DecorationLocation)));
    c.build_dummy_sampler_for_combined_images();
    c.build_combined_image_samplers();
    struct Heap { std::string name, type; };
    std::vector<Heap> heaps;
    for(const auto& sampler:c.get_combined_image_samplers()){
        const auto& type=c.get_type_from_variable(sampler.image_id);
        const std::string dimension=type.image.dim==spv::Dim2D?"2D":type.image.dim==spv::DimCube?"Cube":"";
        if(dimension.empty())throw std::runtime_error("Shader requires unsupported volume/1D texture inputs.");
        const auto name="gsxHeap"+std::to_string(sampler.combined_id);
        heaps.push_back({name,dimension});c.set_name(sampler.combined_id,name);
    }
    std::string result=c.compile();
    std::set<std::string> declared;
    for(const auto& heap:heaps){
        const std::regex declaration("uniform sampler"+heap.type+" "+heap.name+R"(\[16\];)");
        if(!std::regex_search(result,declaration))throw std::runtime_error("Unexpected shader texture heap layout.");
        result=std::regex_replace(result,declaration,"");
        const std::regex use(heap.name+R"(\[([0-9]+)u?\])");
        std::smatch match;
        std::string uniforms;
        while(std::regex_search(result,match,use)){
            const auto slot=std::stoul(match[1]);
            if(slot>=16)throw std::runtime_error("Shader texture slot exceeds the preview binding range.");
            const auto name="gsxTexture"+heap.type+"_"+std::to_string(slot);
            if(declared.insert(name).second)uniforms+="uniform sampler"+heap.type+" "+name+";\n";
            result.replace(match.position(),match.length(),name);
        }
        if(std::regex_search(result,std::regex("\\b"+heap.name+"\\b")))throw std::runtime_error("Dynamic texture indexing requires a runtime binding capture.");
        const auto insertion=result.find("\n\n");
        result.insert(insertion==std::string::npos?result.find('\n')+1:insertion+2,uniforms);
    }
    return result;
}
void Reflect(const ShaderRecompiler& recompiler,bool pixel,ShaderAsset& asset)
{
    for(const auto& pair:recompiler.float4Constants){
        const auto* info=pair.second;
        const auto* name=reinterpret_cast<const char*>(recompiler.constantTableData+info->name);
        if (!info->registerCount || info->registerIndex >= 256 || info->registerCount > 256-info->registerIndex)
            throw std::runtime_error("Shader constant register is out of range.");
        asset.constants.push_back({pixel,uint32_t(info->registerIndex),uint32_t(info->registerCount),name});
    }
}
void ValidateContainer(const std::vector<uint8_t>& bytes,bool pixel)
{
    if(bytes.size()<36 || bytes.size()>8*1024*1024)throw std::runtime_error("Invalid game shader size.");
    const auto word=[&](size_t at){return uint32_t(bytes[at])<<24|uint32_t(bytes[at+1])<<16|uint32_t(bytes[at+2])<<8|bytes[at+3];};
    if(word(0)!=(pixel?0x102a1100u:0x102a1101u)||word(4)<36||word(4)>bytes.size()||word(8)!=bytes.size()-word(4))throw std::runtime_error("Invalid game shader container.");
}
}
bool ShaderCompilerAvailable(){return true;}
ShaderAsset CompileGameShader(const std::vector<uint8_t>& vertex,const std::vector<uint8_t>& pixel)
{
    std::lock_guard<std::mutex> lock(compilerMutex);
    ValidateContainer(vertex,false);ValidateContainer(pixel,true);
    const std::string common(GSXShaderCommon);
    ShaderAsset asset;
    ShaderRecompiler ps;ps.recompile(pixel.data(),common);Reflect(ps,true,asset);asset.pixelHLSL=ps.out;
    ShaderRecompiler vs;vs.recompile(vertex.data(),common);
    if(ps.interpolantMask & ~vs.interpolantMask){vs={};vs.interpolantVariant=ShaderRecompiler::InterpolantVariant::Full;vs.recompile(vertex.data(),common);}
    Reflect(vs,false,asset);asset.vertexHLSL=vs.out;
    asset.vertexGLSL=GLSL(asset.vertexHLSL,false);asset.pixelGLSL=GLSL(asset.pixelHLSL,true);return asset;
}
ShaderAsset CompileShaderEdits(const ShaderAsset& source,const std::string& vertexPath,const std::string& pixelPath)
{
    std::lock_guard<std::mutex> lock(compilerMutex);
    ShaderAsset result=source;
    for(const auto& path:{vertexPath,pixelPath})
        if(std::filesystem::file_size(std::filesystem::u8path(path))>4*1024*1024)
            throw std::runtime_error("Shader source exceeds 4 MiB.");
    const auto vs=ReadFile(std::filesystem::u8path(vertexPath)),ps=ReadFile(std::filesystem::u8path(pixelPath));
    result.vertexHLSL.assign(vs.begin(),vs.end());result.pixelHLSL.assign(ps.begin(),ps.end());
    result.vertexGLSL=GLSL(result.vertexHLSL,false);result.pixelGLSL=GLSL(result.pixelHLSL,true);return result;
}
