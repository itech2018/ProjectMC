#pragma once
#include <SDL3/SDL_gpu.h>
#include <string>
#include <vector>

namespace projectmc::game {

class GpuShaderLoader {
public:
 static std::vector<unsigned char> readFile(const std::string& path);
 static SDL_GPUShader* load(SDL_GPUDevice* device,const std::string& path,SDL_GPUShaderStage stage,
                            Uint32 samplers,Uint32 uniformBuffers);
};

}
