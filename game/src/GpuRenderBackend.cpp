#include "projectmc/game/GpuRenderBackend.hpp"
#include "projectmc/Log.hpp"
#include <string>

namespace projectmc::game {

GpuRenderBackend::~GpuRenderBackend() {
 destroyDepthTarget();
 if(device_) {
  if(window_) SDL_ReleaseWindowFromGPUDevice(device_,window_);
  SDL_DestroyGPUDevice(device_);
 }
}

bool GpuRenderBackend::initialize(SDL_Window* window) {
 window_=window;
 if(!window_) return false;

 // Prefer the platform's best supported backend. SDL will normally select
 // Direct3D 12 on modern Windows, with other supported APIs available on
 // other platforms.
 device_=SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV|SDL_GPU_SHADERFORMAT_DXIL|SDL_GPU_SHADERFORMAT_MSL,false,nullptr);
 if(!device_) {
  projectmc::log(projectmc::LogLevel::Warning,std::string("SDL GPU device unavailable: ")+SDL_GetError());
  return false;
 }
 if(!SDL_ClaimWindowForGPUDevice(device_,window_)) {
  projectmc::log(projectmc::LogLevel::Warning,std::string("Could not claim window for SDL GPU: ")+SDL_GetError());
  SDL_DestroyGPUDevice(device_);
  device_=nullptr;
  return false;
 }

 SDL_GetWindowSizeInPixels(window_,&width_,&height_);
 if(!createDepthTarget()) return false;
 return true;
}

void GpuRenderBackend::destroyDepthTarget() {
 if(device_&&depthTexture_) SDL_ReleaseGPUTexture(device_,depthTexture_);
 depthTexture_=nullptr;
}

bool GpuRenderBackend::createDepthTarget() {
 destroyDepthTarget();
 if(!device_||width_<=0||height_<=0) return false;
 SDL_GPUTextureCreateInfo info{};
 info.type=SDL_GPU_TEXTURETYPE_2D;
 info.format=SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
 info.usage=SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
 info.width=static_cast<Uint32>(width_);
 info.height=static_cast<Uint32>(height_);
 info.layer_count_or_depth=1;
 info.num_levels=1;
 info.sample_count=SDL_GPU_SAMPLECOUNT_1;
 depthTexture_=SDL_CreateGPUTexture(device_,&info);
 if(!depthTexture_) {
  projectmc::log(projectmc::LogLevel::Warning,std::string("Could not create GPU depth target: ")+SDL_GetError());
  return false;
 }
 return true;
}

void GpuRenderBackend::resize(int width,int height) {
 if(width<=0||height<=0) return;
 if(width_==width&&height_==height) return;
 width_=width;
 height_=height;
 createDepthTarget();
}

void GpuRenderBackend::beginFrame(const Camera& camera) {
 matrices_.update(camera,width_,height_);
 commandBuffer_=nullptr;
 swapchainTexture_=nullptr;
 if(!device_||!window_) return;

 commandBuffer_=SDL_AcquireGPUCommandBuffer(device_);
 if(!commandBuffer_) {
  projectmc::log(projectmc::LogLevel::Warning,std::string("Could not acquire GPU command buffer: ")+SDL_GetError());
  return;
 }

 Uint32 swapWidth=0,swapHeight=0;
 if(!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer_,window_,&swapchainTexture_,&swapWidth,&swapHeight)) {
  projectmc::log(projectmc::LogLevel::Warning,std::string("Could not acquire GPU swapchain texture: ")+SDL_GetError());
  SDL_CancelGPUCommandBuffer(commandBuffer_);
  commandBuffer_=nullptr;
  return;
 }
 if(!swapchainTexture_) return;

 if(static_cast<int>(swapWidth)!=width_||static_cast<int>(swapHeight)!=height_) {
  width_=static_cast<int>(swapWidth);
  height_=static_cast<int>(swapHeight);
  createDepthTarget();
  matrices_.update(camera,width_,height_);
 }

 SDL_GPUColorTargetInfo color{};
 color.texture=swapchainTexture_;
 color.clear_color={0.41f,0.69f,0.90f,1.0f};
 color.load_op=SDL_GPU_LOADOP_CLEAR;
 color.store_op=SDL_GPU_STOREOP_STORE;

 SDL_GPUDepthStencilTargetInfo depth{};
 depth.texture=depthTexture_;
 depth.clear_depth=1.0f;
 depth.load_op=SDL_GPU_LOADOP_CLEAR;
 depth.store_op=SDL_GPU_STOREOP_DONT_CARE;
 depth.stencil_load_op=SDL_GPU_LOADOP_DONT_CARE;
 depth.stencil_store_op=SDL_GPU_STOREOP_DONT_CARE;
 depth.cycle=false;
 depth.clear_stencil=0;

 SDL_GPURenderPass* pass=SDL_BeginGPURenderPass(commandBuffer_,&color,1,depthTexture_?&depth:nullptr);
 if(pass) SDL_EndGPURenderPass(pass);
}

void GpuRenderBackend::endFrame() {
 if(!commandBuffer_) return;
 if(!SDL_SubmitGPUCommandBuffer(commandBuffer_))
  projectmc::log(projectmc::LogLevel::Warning,std::string("Could not submit GPU frame: ")+SDL_GetError());
 commandBuffer_=nullptr;
 swapchainTexture_=nullptr;
}

}
