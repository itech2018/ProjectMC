#include "projectmc/game/GpuRenderBackend.hpp"
#include "projectmc/game/GpuShaderLoader.hpp"
#include "projectmc/Log.hpp"
#include <string>
#include <cstring>
#include <filesystem>

namespace projectmc::game {

GpuRenderBackend::~GpuRenderBackend() {
 if(device_&&worldPipeline_) SDL_ReleaseGPUGraphicsPipeline(device_,worldPipeline_);
 if(device_&&worldVertexShader_) SDL_ReleaseGPUShader(device_,worldVertexShader_);
 if(device_&&worldFragmentShader_) SDL_ReleaseGPUShader(device_,worldFragmentShader_);
 worldPipeline_=nullptr;
 worldVertexShader_=nullptr;
 worldFragmentShader_=nullptr;
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
 if(!loadWorldShaders())
  projectmc::log(projectmc::LogLevel::Warning,"GPU device is ready, but compiled voxel shaders are not available yet.");
 return true;
}

std::string GpuRenderBackend::shaderPath(const char* stem) const {
 const SDL_GPUShaderFormat formats=SDL_GetGPUShaderFormats(device_);
 const char* extension=nullptr;
 if(formats&SDL_GPU_SHADERFORMAT_DXIL) extension=".dxil";
 else if(formats&SDL_GPU_SHADERFORMAT_SPIRV) extension=".spv";
 else if(formats&SDL_GPU_SHADERFORMAT_MSL) extension=".msl";
 if(!extension) return {};

 const char* base=SDL_GetBasePath();
 std::filesystem::path root=(base&&*base)?std::filesystem::path(base):std::filesystem::current_path();
 return (root/"shaders"/(std::string(stem)+extension)).string();
}

bool GpuRenderBackend::loadWorldShaders() {
 if(!device_) return false;
 const std::string vertexPath=shaderPath("voxel.vert");
 const std::string fragmentPath=shaderPath("voxel.frag");
 if(vertexPath.empty()||fragmentPath.empty()) return false;

 worldVertexShader_=GpuShaderLoader::load(device_,vertexPath,SDL_GPU_SHADERSTAGE_VERTEX,0,1);
 worldFragmentShader_=GpuShaderLoader::load(device_,fragmentPath,SDL_GPU_SHADERSTAGE_FRAGMENT,1,0);
 if(!worldVertexShader_||!worldFragmentShader_) {
  if(worldVertexShader_) SDL_ReleaseGPUShader(device_,worldVertexShader_);
  if(worldFragmentShader_) SDL_ReleaseGPUShader(device_,worldFragmentShader_);
  worldVertexShader_=nullptr;
  worldFragmentShader_=nullptr;
  return false;
 }
 if(!createWorldPipeline(worldVertexShader_,worldFragmentShader_)) return false;
 projectmc::log(projectmc::LogLevel::Info,"GPU voxel shaders and world pipeline loaded.");
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

bool GpuRenderBackend::createWorldPipeline(SDL_GPUShader* vertexShader,SDL_GPUShader* fragmentShader) {
 if(!device_||!vertexShader||!fragmentShader) return false;
 if(worldPipeline_) SDL_ReleaseGPUGraphicsPipeline(device_,worldPipeline_);

 SDL_GPUVertexBufferDescription buffer{};
 buffer.slot=0;
 buffer.pitch=7*sizeof(float);
 buffer.input_rate=SDL_GPU_VERTEXINPUTRATE_VERTEX;

 SDL_GPUVertexAttribute attrs[4]{};
 attrs[0]={0,0,SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,0};
 attrs[1]={1,0,SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,3*sizeof(float)};
 attrs[2]={2,0,SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,5*sizeof(float)};
 attrs[3]={3,0,SDL_GPU_VERTEXELEMENTFORMAT_FLOAT,6*sizeof(float)};

 SDL_GPUColorTargetDescription color{};
 color.format=SDL_GetGPUSwapchainTextureFormat(device_,window_);

 SDL_GPUGraphicsPipelineCreateInfo info{};
 info.vertex_shader=vertexShader;
 info.fragment_shader=fragmentShader;
 info.vertex_input_state.vertex_buffer_descriptions=&buffer;
 info.vertex_input_state.num_vertex_buffers=1;
 info.vertex_input_state.vertex_attributes=attrs;
 info.vertex_input_state.num_vertex_attributes=4;
 info.primitive_type=SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
 info.rasterizer_state.fill_mode=SDL_GPU_FILLMODE_FILL;
 info.rasterizer_state.cull_mode=SDL_GPU_CULLMODE_BACK;
 info.rasterizer_state.front_face=SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
 info.depth_stencil_state.compare_op=SDL_GPU_COMPAREOP_LESS;
 info.depth_stencil_state.enable_depth_test=true;
 info.depth_stencil_state.enable_depth_write=true;
 info.target_info.color_target_descriptions=&color;
 info.target_info.num_color_targets=1;
 info.target_info.depth_stencil_format=SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
 info.target_info.has_depth_stencil_target=true;

 worldPipeline_=SDL_CreateGPUGraphicsPipeline(device_,&info);
 if(!worldPipeline_) projectmc::log(projectmc::LogLevel::Warning,std::string("Could not create GPU world pipeline: ")+SDL_GetError());
 return worldPipeline_!=nullptr;
}

bool GpuRenderBackend::uploadMesh(const void* vertices,Uint32 vertexBytes,const void* indices,Uint32 indexBytes,Uint32 indexCount,BufferPair& out) {
 if(!device_||!vertices||!indices||vertexBytes==0||indexBytes==0) return false;

 releaseMesh(out);

 SDL_GPUBufferCreateInfo vbInfo{};
 vbInfo.usage=SDL_GPU_BUFFERUSAGE_VERTEX;
 vbInfo.size=vertexBytes;
 out.vertex=SDL_CreateGPUBuffer(device_,&vbInfo);

 SDL_GPUBufferCreateInfo ibInfo{};
 ibInfo.usage=SDL_GPU_BUFFERUSAGE_INDEX;
 ibInfo.size=indexBytes;
 out.index=SDL_CreateGPUBuffer(device_,&ibInfo);

 if(!out.vertex||!out.index) {
  projectmc::log(projectmc::LogLevel::Warning,std::string("Could not create chunk GPU buffers: ")+SDL_GetError());
  releaseMesh(out);
  return false;
 }

 const Uint32 total=vertexBytes+indexBytes;
 SDL_GPUTransferBufferCreateInfo transferInfo{};
 transferInfo.usage=SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
 transferInfo.size=total;
 SDL_GPUTransferBuffer* transfer=SDL_CreateGPUTransferBuffer(device_,&transferInfo);
 if(!transfer) {
  projectmc::log(projectmc::LogLevel::Warning,std::string("Could not create GPU upload buffer: ")+SDL_GetError());
  releaseMesh(out);
  return false;
 }

 void* mapped=SDL_MapGPUTransferBuffer(device_,transfer,false);
 if(!mapped) {
  projectmc::log(projectmc::LogLevel::Warning,std::string("Could not map GPU upload buffer: ")+SDL_GetError());
  SDL_ReleaseGPUTransferBuffer(device_,transfer);
  releaseMesh(out);
  return false;
 }
 std::memcpy(mapped,vertices,vertexBytes);
 std::memcpy(static_cast<unsigned char*>(mapped)+vertexBytes,indices,indexBytes);
 SDL_UnmapGPUTransferBuffer(device_,transfer);

 SDL_GPUCommandBuffer* cmd=SDL_AcquireGPUCommandBuffer(device_);
 if(!cmd) {
  SDL_ReleaseGPUTransferBuffer(device_,transfer);
  releaseMesh(out);
  return false;
 }
 SDL_GPUCopyPass* copy=SDL_BeginGPUCopyPass(cmd);
 if(!copy) {
  SDL_CancelGPUCommandBuffer(cmd);
  SDL_ReleaseGPUTransferBuffer(device_,transfer);
  releaseMesh(out);
  return false;
 }

 SDL_GPUTransferBufferLocation vertexSrc{transfer,0};
 SDL_GPUBufferRegion vertexDst{out.vertex,0,vertexBytes};
 SDL_UploadToGPUBuffer(copy,&vertexSrc,&vertexDst,false);

 SDL_GPUTransferBufferLocation indexSrc{transfer,vertexBytes};
 SDL_GPUBufferRegion indexDst{out.index,0,indexBytes};
 SDL_UploadToGPUBuffer(copy,&indexSrc,&indexDst,false);
 SDL_EndGPUCopyPass(copy);

 if(!SDL_SubmitGPUCommandBuffer(cmd)) {
  projectmc::log(projectmc::LogLevel::Warning,std::string("Could not submit chunk GPU upload: ")+SDL_GetError());
  SDL_ReleaseGPUTransferBuffer(device_,transfer);
  releaseMesh(out);
  return false;
 }

 SDL_ReleaseGPUTransferBuffer(device_,transfer);
 out.indexCount=indexCount;
 return true;
}

void GpuRenderBackend::releaseMesh(BufferPair& mesh) {
 if(device_) {
  if(mesh.vertex) SDL_ReleaseGPUBuffer(device_,mesh.vertex);
  if(mesh.index) SDL_ReleaseGPUBuffer(device_,mesh.index);
 }
 mesh={};
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
 if(commandBuffer_&&worldPipeline_)
  SDL_PushGPUVertexUniformData(commandBuffer_,0,matrices_.viewProjection.m.data(),static_cast<Uint32>(sizeof(matrices_.viewProjection.m)));
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
