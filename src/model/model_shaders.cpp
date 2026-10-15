#include "model/model_preview.h"
#include "platform/file_dialog.h"
#include "platform/file_io.h"
#include "Resource/RZTexture.h"
#include "texture/cube_texture.h"
#include "imgui.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <chrono>
#include <stdexcept>

void ModelPreview::InstallShader(size_t index,const ShaderAsset& asset)
{
    if(index>=materials.size())throw std::runtime_error("Select a material first.");
    if(materials[index].uvSet!=0)throw std::runtime_error("Game shader preview currently supports base UV set 0.");
    for(const auto& mesh:meshes)if(mesh.material==index && !mesh.shaderVertexError.empty())throw std::runtime_error(mesh.shaderVertexError);
    materialShaders[index]->Load(asset);
    textured=true;renderDirty=true;
    Log("Game shader preview loaded for "+materials[index].name+" (package unchanged).");
}
void ModelPreview::CompileGamePass(size_t index,size_t passIndex)
{
    if(shaderJob.valid())throw std::runtime_error("A shader is already compiling.");
    if(index>=materials.size() || passIndex>=materials[index].passes.size())throw std::runtime_error("Select a material pass first.");
    const auto& pass=materials[index].passes[passIndex];
    if(pass.set!=0)throw std::runtime_error("Start with a static pass; skinned/morph inputs require runtime pose data.");
    auto vertex=ModelShaderContainer(materials[index],pass.vertex),pixel=ModelShaderContainer(materials[index],pass.pixel);
    if(vertex.empty()||pixel.empty())throw std::runtime_error("This pass refers to shaders not embedded in the package.");
    shaderJobMaterial=index;shaderJobSelected=selected;shaderJobPackage=package;shaderJobRevision=revision;
    shaderJobPath.clear();
    shaderJobLabel=materials[index].name+" / slot "+std::to_string(pass.slot)+" / set "+std::to_string(pass.set);
    shaderJob=std::async(std::launch::async,[vertex=std::move(vertex),pixel=std::move(pixel)](){return CompileGameShader(vertex,pixel);});
    Log("Converting "+shaderJobLabel+" with reeot-XenosRecomp.");
}
void ModelPreview::PollShaderJob(const PackageSession& session)
{
    if(!shaderJob.valid()||shaderJob.wait_for(std::chrono::seconds(0))!=std::future_status::ready)return;
    try {
        auto asset=shaderJob.get();
        if(session.package.get()!=shaderJobPackage || session.selected!=shaderJobSelected || session.revision!=shaderJobRevision)return;
        auto& shader=*materialShaders[shaderJobMaterial];
        const bool captured=shader.captured, camera=shader.capturedCamera;
        const auto constants=shader.capturedConstants;
        InstallShader(shaderJobMaterial,asset);shader.path=shaderJobPath;shader.label=shaderJobLabel;
        Log("Active shader: "+shaderJobLabel);
        if(!shaderJobPath.empty()){
            shader.captured=captured;shader.capturedCamera=camera;shader.capturedConstants=constants;
        }
    }catch(const std::exception& ex){Log("Shader conversion failed ["+shaderJobLabel+"]; previous preview kept: "+std::string(ex.what()));}
}
void ModelPreview::LoadShaderPreview(size_t index,const std::string& path)
{
    auto asset=ReadShaderAsset(path);InstallShader(index,asset);materialShaders[index]->path=path;materialShaders[index]->label="Imported shader preview";
}
void ModelPreview::ReloadShader(size_t index)
{
    if(shaderJob.valid())throw std::runtime_error("A shader is already compiling.");
    if(index>=materialShaders.size()||materialShaders[index]->path.empty())throw std::runtime_error("Save the editable shader preview first.");
    shaderJobMaterial=index;shaderJobSelected=selected;shaderJobPackage=package;shaderJobRevision=revision;
    const auto asset=materialShaders[index]->asset;const auto path=materialShaders[index]->path;
    shaderJobPath=path;shaderJobLabel=materialShaders[index]->label;
    shaderJob=std::async(std::launch::async,[asset,path](){return CompileShaderEdits(asset,path+".vert.hlsl",path+".frag.hlsl");});
    Log("Compiling shader edits: "+path);
}
unsigned int ModelPreview::ShaderTextureID(size_t index,size_t slot,unsigned target)
{
    const auto& sampler=materials[index].samplers[slot];auto& cached=shaderTextures[index][slot];
    const bool base=target==GL_TEXTURE_2D && sampler.crc==materials[index].textureCRC;
    if(!cached.attempted){
        cached.attempted=true;cached.target=target;
        if(sampler.texture)try{
            if(target==0x8513){
                const auto* post=RZTexture::FindRootAtOffset(*package,sampler.texture->GetDataOffset());
                const auto* data=post?post->FindChild(Texture_Data):nullptr;
                if(!data)throw std::runtime_error("No cube Texture_Data at the resource offset.");
                const auto cube=DecodeCubeTexture(*data);cached.format=cube.format;
                cached.texture=UploadCube(cube.faces,cube.size);
            }else{
                const auto texture=RZTexture::Load(*package,*sampler.texture);cached.format=texture.desc.d3dFormat;
                if(!base){
                    if(!texture.desc.Is2D()||texture.desc.width>4096||texture.desc.height>4096)throw std::runtime_error("Only 2D textures up to 4096 are supported.");
                    cached.texture=UploadTexture(texture.DecodeLevel0(),texture.desc.width,texture.desc.height);
                    glBindTexture(GL_TEXTURE_2D,cached.texture);
                    const int mode=sampler.addressMode==1?0x812F:GL_REPEAT;
                    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,mode);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,mode);
                }
            }
        }catch(const std::exception& ex){Log("Shader sampler "+std::to_string(slot)+": "+ex.what());}
        else Log("Shader texture CRC "+std::to_string(sampler.crc)+" is not in this package.");
    }
    if(cached.target!=target)return 0;
    return base?MaterialTextureID(index):cached.texture;
}
void ModelPreview::DrawShaderActions()
{
    if(materialIndex<0||size_t(materialIndex)>=materials.size()||!ImGui::CollapsingHeader("Shader previews"))return;
    auto& material=materials[materialIndex];auto& shader=*materialShaders[materialIndex];
    shaderPassIndex=std::clamp(shaderPassIndex,0,std::max(0,int(material.passes.size())-1));
    const auto label=[](const ModelPass& pass){return "Technique "+std::to_string(pass.slot)+(pass.set==0?" / static":" / runtime pose");};
    if(!material.passes.empty()){
        ImGui::SetNextItemWidth(-1);
        if(ImGui::BeginCombo("##Shader technique",label(material.passes[shaderPassIndex]).c_str())){
            for(size_t i=0;i<material.passes.size();++i)if(ImGui::Selectable(label(material.passes[i]).c_str(),shaderPassIndex==int(i)))shaderPassIndex=int(i);
            ImGui::EndCombo();
        }
    }
    const bool busy=shaderJob.valid();
    const auto* pass=material.passes.empty()?nullptr:&material.passes[shaderPassIndex];
    const bool embedded=pass && pass->vertex.virtualSize>pass->vertex.headerSize && pass->pixel.virtualSize>pass->pixel.headerSize;
    if(pass && !embedded)ImGui::TextDisabled("No embedded vertex/pixel pair in this slot.");
    if(busy)ImGui::TextDisabled("Compiling shaders...");
    try{
        ImGui::BeginDisabled(busy||!ShaderCompilerAvailable()||!embedded||pass->set!=0);
        const bool compile=ImGui::Button("Preview game shaders",ImVec2(-1,0));
        ImGui::EndDisabled();
        ImGui::SetItemTooltip("Converts the selected static pass with reeot-XenosRecomp. Build with GSX_SHADER_COMPILER=ON to enable conversion.");
        if(compile)CompileGamePass(materialIndex,shaderPassIndex);
        ImGui::BeginDisabled(busy);
        const bool load=ImGui::Button("Load shader preview...",ImVec2(-1,0));
        ImGui::EndDisabled();
        if(load){const auto path=PickFile(glfwGetCurrentContext(),"GSX shader preview","gsxshader",false);if(!path.empty())LoadShaderPreview(materialIndex,path);}
        if(shader.program){
            if(!shader.label.empty())ImGui::TextWrapped("Active: %s",shader.label.c_str());
            if(ImGui::Checkbox("Use game shader preview",&shader.enabled))renderDirty=true;
            ImGui::TextDisabled(shader.captured?"Captured constants":"Studio lighting defaults");
            if(ImGui::Button("Save editable shader...",ImVec2(-1,0))){
                const auto path=PickFile(glfwGetCurrentContext(),"GSX shader preview","gsxshader",true,SanitizeFileName(material.name)+".gsxshader");
                if(!path.empty()){WriteShaderAsset(shader.asset,path);shader.path=path;Log("Editable GSX shader saved: "+path+" (edit .vert.hlsl / .frag.hlsl, then reload).");}
            }
            ImGui::BeginDisabled(busy||shader.path.empty()||!ShaderCompilerAvailable());
            const bool reload=ImGui::Button("Reload HLSL edits",ImVec2(-1,0));
            ImGui::EndDisabled();if(reload)ReloadShader(materialIndex);
            if(ImGui::Button("Load game constants...",ImVec2(-1,0))){
                const auto path=PickFile(glfwGetCurrentContext(),"GSX draw constants","bin",false);
                if(!path.empty()){shader.LoadConstants(path);renderDirty=true;Log("Game draw constants loaded: "+path);}
            }
            if(shader.captured){
                if(ImGui::Checkbox("Use captured camera",&shader.capturedCamera))renderDirty=true;
                if(ImGui::Button("Use studio constants",ImVec2(-1,0))){shader.captured=false;renderDirty=true;}
            }
        }
    }catch(const std::exception& ex){Log("Shader preview: "+std::string(ex.what()));}
}
