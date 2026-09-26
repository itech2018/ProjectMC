#include "projectmc/game/GpuShaderLoader.hpp"
#include "projectmc/Log.hpp"
#include <fstream>
#include <iterator>

namespace projectmc::game {

std::vector<unsigned char> GpuShaderLoader::readFile(const std::string& path) {
 std::ifstream in(path,std::ios::binary);
 if(!in) return {};
 return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
}

SDL_GPUShader* GpuShaderLoader::load(SDL_GPUDevice* device,const std::string& path,SDL_GPUShaderStage stage,
                                     Uint32 samplers,Uint32 uniformBuffers) {
 auto code=readFile(path);
 if(!device||code.empty()) return nullptr;

 SDL_GPUShaderFormat format=SDL_GPU_SHADERFORMAT_INVALID;
 const char* entry="main";
 if(path.ends_with(".spv")) format=SDL_GPU_SHADERFORMAT_SPIRV;
 else if(path.ends_with(".dxil")) format=SDL_GPU_SHADERFORMAT_DXIL;
 else if(path.ends_with(".msl")) format=SDL_GPU_SHADERFORMAT_MSL;
 else return nullptr;

 SDL_GPUShaderCreateInfo info{};
 info.code_size=code.size();
 info.code=code.data();
 info.entrypoint=entry;
 info.format=format;
 info.stage=stage;
 info.num_samplers=samplers;
 info.num_uniform_buffers=uniformBuffers;
 auto* shader=SDL_CreateGPUShader(device,&info);
 if(!shader) projectmc::log(projectmc::LogLevel::Warning,"GPU shader load failed: "+path);
 return shader;
}

}
