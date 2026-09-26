#include "projectmc/game/Application.hpp"
#include "projectmc/Log.hpp"
#include "projectmc/game/SdlRenderBackend.hpp"
#include "projectmc/game/GpuRenderBackend.hpp"
#include <chrono>
#include <cmath>
#include <utility>
#include <sstream>
#include <iomanip>
namespace projectmc::game {
constexpr float PI=3.14159265358979323846f;
Application::Application(projectmc::GameConfig c):config_(std::move(c)){}
Application::~Application(){
 if(auto* gpu=dynamic_cast<GpuRenderBackend*>(renderBackend_.get()))
  chunkRenderer_.releaseGpuMeshes(*gpu);
 renderBackend_.reset();
 if(renderer_){SDL_DestroyRenderer(renderer_);renderer_=nullptr;}
 if(window_){SDL_DestroyWindow(window_);window_=nullptr;}
 SDL_Quit();
}
bool Application::initialize(){
 projectmc::log(projectmc::LogLevel::Info,"Preparing SDL video...");
#ifdef _WIN32
 // Default to the native Win32 backend, but allow PROJECTMC_SDL_VIDEO_DRIVER
 // to override it for diagnosing driver-specific startup failures.
 const char*requested=SDL_getenv("PROJECTMC_SDL_VIDEO_DRIVER");
 if(!requested||!*requested)requested="windows";
 projectmc::log(projectmc::LogLevel::Info,"Requested SDL video driver: "+std::string(requested));
 if(!SDL_SetHint(SDL_HINT_VIDEO_DRIVER,requested))
  projectmc::log(projectmc::LogLevel::Warning,"Could not set SDL video-driver hint.");
#endif
 int videoDrivers=SDL_GetNumVideoDrivers();
 projectmc::log(projectmc::LogLevel::Info,"SDL reports "+std::to_string(videoDrivers)+" video driver(s).");
 for(int i=0;i<videoDrivers;++i){const char*d=SDL_GetVideoDriver(i);if(d)projectmc::log(projectmc::LogLevel::Info,"SDL video driver: "+std::string(d));}
 projectmc::log(projectmc::LogLevel::Info,"Initialising SDL video + events...");
 if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)){projectmc::log(projectmc::LogLevel::Error,std::string("SDL init failed: ")+SDL_GetError());return false;}
 const char*activeDriver=SDL_GetCurrentVideoDriver();
 projectmc::log(projectmc::LogLevel::Info,std::string("SDL video ready: ")+(activeDriver?activeDriver:"unknown"));
 projectmc::log(projectmc::LogLevel::Info,"Creating game window...");
 SDL_WindowFlags f=SDL_WINDOW_RESIZABLE;if(config_.fullscreen)f|=SDL_WINDOW_FULLSCREEN;
 window_=SDL_CreateWindow(config_.title.c_str(),config_.width,config_.height,f);
 if(!window_){projectmc::log(projectmc::LogLevel::Error,std::string("SDL_CreateWindow failed: ")+SDL_GetError());return false;}
 SDL_ShowWindow(window_);SDL_RaiseWindow(window_);
 projectmc::log(projectmc::LogLevel::Info,std::string("Keyboard focus after window creation: ")+(SDL_GetKeyboardFocus()==window_?"yes":"no"));
 const char* compatibility=SDL_getenv("PROJECTMC_COMPAT_RENDERER");
 gpuMode_=!(compatibility&&std::string(compatibility)=="1");
 if(gpuMode_){
  projectmc::log(projectmc::LogLevel::Info,"Starting primary SDL_GPU renderer.");
  auto gpu=std::make_unique<GpuRenderBackend>();
  if(!gpu->initialize(window_)){
   projectmc::log(projectmc::LogLevel::Error,"SDL_GPU renderer initialisation failed. Set PROJECTMC_COMPAT_RENDERER=1 to use the compatibility renderer.");
   return false;
  }
  renderBackend_=std::move(gpu);
 }else{
  projectmc::log(projectmc::LogLevel::Info,"Creating SDL compatibility renderer...");
  renderer_=SDL_CreateRenderer(window_,nullptr);
  if(!renderer_){projectmc::log(projectmc::LogLevel::Error,std::string("SDL_CreateRenderer failed: ")+SDL_GetError());return false;}
  SDL_SetRenderVSync(renderer_,config_.vsync?1:0);
  auto fallback=std::make_unique<SdlRenderBackend>();
  if(!fallback->initialize(window_)){projectmc::log(projectmc::LogLevel::Error,"Render backend initialisation failed.");return false;}
  renderBackend_=std::move(fallback);
 }
 projectmc::log(projectmc::LogLevel::Info,std::string("Render backend: ")+renderBackend_->name());
 projectmc::log(projectmc::LogLevel::Info,"Creating texture atlas...");
 if(gpuMode_){
  if(!atlas_.createCpu()){projectmc::log(projectmc::LogLevel::Error,"CPU texture atlas generation failed.");return false;}
  auto* gpu=dynamic_cast<GpuRenderBackend*>(renderBackend_.get());
  if(!gpu||!gpu->uploadAtlas(atlas_)){projectmc::log(projectmc::LogLevel::Error,std::string("GPU texture atlas upload failed: ")+SDL_GetError());return false;}
 }else{
  if(!atlas_.create(renderer_)){projectmc::log(projectmc::LogLevel::Error,std::string("Texture atlas failed: ")+SDL_GetError());return false;}
 }
 projectmc::log(projectmc::LogLevel::Info,"Generating spawn terrain...");
 world_.generateTerrain(2);
 // Pump the window once before grabbing the mouse. On Windows this avoids capturing
 // input while the SDL window is still being created/activated.
 SDL_PumpEvents();SDL_RaiseWindow(window_);
 if(!SDL_SetWindowRelativeMouseMode(window_,true))
  projectmc::log(projectmc::LogLevel::Warning,std::string("Relative mouse mode unavailable: ")+SDL_GetError());
 running_=true;
 projectmc::log(projectmc::LogLevel::Info,"Game initialised - entering main loop.");
 return true;
}
void Application::processEvents(){
 input_.mouseDeltaX=input_.mouseDeltaY=0;
 input_.removeBlock=input_.placeBlock=false;
 input_.hotbarSelection=-1;

 SDL_Event e;
 while(SDL_PollEvent(&e)){
  if(e.type==SDL_EVENT_QUIT)running_=false;

  if(e.type==SDL_EVENT_WINDOW_RESIZED&&renderBackend_) renderBackend_->resize(e.window.data1,e.window.data2);

  if(e.type==SDL_EVENT_WINDOW_FOCUS_GAINED)
   SDL_SetWindowRelativeMouseMode(window_,true);

  if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST){
   SDL_SetWindowRelativeMouseMode(window_,false);
   input_.mouseDeltaX=input_.mouseDeltaY=0;
   input_.forward=input_.backward=input_.left=input_.right=false;
   input_.jump=input_.descend=input_.sprint=false;
  }

  if(e.type==SDL_EVENT_KEY_DOWN||e.type==SDL_EVENT_KEY_UP){
   const bool down=e.type==SDL_EVENT_KEY_DOWN;
   switch(e.key.scancode){
    case SDL_SCANCODE_W: input_.forward=down; break;
    case SDL_SCANCODE_S: input_.backward=down; break;
    case SDL_SCANCODE_A: input_.left=down; break;
    case SDL_SCANCODE_D: input_.right=down; break;
    case SDL_SCANCODE_SPACE: input_.jump=down; break;
    case SDL_SCANCODE_LCTRL:
    case SDL_SCANCODE_RCTRL: input_.descend=down; break;
    case SDL_SCANCODE_LSHIFT:
    case SDL_SCANCODE_RSHIFT: input_.sprint=down; break;
    default: break;
   }

   if(down&&!e.key.repeat){
    if(e.key.key==SDLK_ESCAPE)running_=false;
    if(e.key.key>=SDLK_1&&e.key.key<=SDLK_5)
     input_.hotbarSelection=(int)(e.key.key-SDLK_1);
   }
  }

  if(e.type==SDL_EVENT_MOUSE_MOTION){
   input_.mouseDeltaX+=e.motion.xrel;
   input_.mouseDeltaY+=e.motion.yrel;
  }

  if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN){
   if(e.button.button==SDL_BUTTON_LEFT)input_.removeBlock=true;
   if(e.button.button==SDL_BUTTON_RIGHT)input_.placeBlock=true;
  }
 }
}
void Application::interact(bool place){
 auto h=raycastBlocks(world_,camera_);
 if(!h.hit)return;
 if(place){
  const int bx=h.previousX,by=h.previousY,bz=h.previousZ;
  const auto& def=world_.blocks().get(selectedBlock_);
  if(def.solid){
   constexpr float playerHalfWidth=.3f;
   constexpr float playerHeight=1.8f;
   const float pMinX=player_.position.x-playerHalfWidth,pMaxX=player_.position.x+playerHalfWidth;
   const float pMinY=player_.position.y,pMaxY=player_.position.y+playerHeight;
   const float pMinZ=player_.position.z-playerHalfWidth,pMaxZ=player_.position.z+playerHalfWidth;
   const bool overlaps=
    pMaxX>bx&&pMinX<bx+1.0f&&
    pMaxY>by&&pMinY<by+1.0f&&
    pMaxZ>bz&&pMinZ<bz+1.0f;
   if(overlaps)return;
  }
  world_.setBlock(bx,by,bz,selectedBlock_);
 }else world_.setBlock(h.x,h.y,h.z,0);
}
void Application::update(double dt){constexpr float mouseSensitivity=.09f;
 camera_.yaw+=input_.mouseDeltaX*mouseSensitivity;
 camera_.pitch-=input_.mouseDeltaY*mouseSensitivity;
 if(camera_.yaw>180.0f)camera_.yaw-=360.0f;
 if(camera_.yaw<-180.0f)camera_.yaw+=360.0f;if(camera_.pitch>89)camera_.pitch=89;if(camera_.pitch<-89)camera_.pitch=-89;player_.update(dt,input_,world_,camera_.yaw);camera_.position=player_.eyePosition();world_.updateStreaming(player_.position.x,player_.position.z,2);if(input_.hotbarSelection>=0){selectedSlot_=input_.hotbarSelection;const char* names[5]={"stone","dirt","grass","sand","water"};selectedBlock_=world_.blocks().id(names[selectedSlot_]);}if(input_.removeBlock)interact(false);if(input_.placeBlock)interact(true);}
void Application::render(){
 chunkRenderer_.syncMeshes(world_,atlas_);
 if(gpuMode_){
  auto* gpu=dynamic_cast<GpuRenderBackend*>(renderBackend_.get());
  if(!gpu) return;
  chunkRenderer_.syncGpuMeshes(*gpu);
  int w=0,h=0;
  SDL_GetWindowSizeInPixels(window_,&w,&h);
  gpu->beginFrame(camera_);
  chunkRenderer_.renderGpuWorld(*gpu,camera_,w,h);
  const auto hit=raycastBlocks(world_,camera_);
  if(hit.hit) gpu->drawSelection(hit.x,hit.y,hit.z);
  gpu->drawHud(selectedSlot_);
  gpu->endFrame();
  return;
 }
 if(renderBackend_)renderBackend_->beginFrame(camera_);
 SDL_SetRenderDrawColor(renderer_,105,175,230,255);SDL_RenderClear(renderer_);
 int w=0,h=0;SDL_GetRenderOutputSize(renderer_,&w,&h);
 chunkRenderer_.renderWorld(renderer_,world_,atlas_,camera_,w,h);
 auto hit=raycastBlocks(world_,camera_);if(hit.hit)chunkRenderer_.renderSelection(renderer_,hit.x,hit.y,hit.z,camera_,w,h);
 const float slot=42,gap=4,total=5*slot+4*gap,start=w*.5f-total*.5f,y=h-58;
 for(int i=0;i<5;++i){SDL_FRect box{start+i*(slot+gap),y,slot,slot};if(i==selectedSlot_)SDL_SetRenderDrawColor(renderer_,255,255,255,255);else SDL_SetRenderDrawColor(renderer_,70,70,70,220);SDL_RenderRect(renderer_,&box);SDL_FRect inner{box.x+5,box.y+5,box.w-10,box.h-10};switch(i){case 0:SDL_SetRenderDrawColor(renderer_,120,120,125,255);break;case 1:SDL_SetRenderDrawColor(renderer_,120,80,50,255);break;case 2:SDL_SetRenderDrawColor(renderer_,90,155,70,255);break;case 3:SDL_SetRenderDrawColor(renderer_,210,195,135,255);break;default:SDL_SetRenderDrawColor(renderer_,60,120,210,255);}SDL_RenderFillRect(renderer_,&inner);}
 SDL_SetRenderDrawColor(renderer_,255,255,255,255);SDL_RenderLine(renderer_,w/2-6,h/2,w/2+6,h/2);SDL_RenderLine(renderer_,w/2,h/2-6,w/2,h/2+6);
 SDL_RenderPresent(renderer_);if(renderBackend_)renderBackend_->endFrame();
}
int Application::run(){
 using C=std::chrono::steady_clock;
 auto p=C::now();
 while(running_){
  auto n=C::now();
  std::chrono::duration<double>d=n-p;p=n;
  processEvents();update(d.count());render();

  fpsAccumulator_+=d.count();
  ++fpsFrames_;
  if(fpsAccumulator_>=0.5){
   displayedFps_=fpsFrames_/fpsAccumulator_;
   fpsAccumulator_=0.0;fpsFrames_=0;
   const auto s=chunkRenderer_.stats();
   std::ostringstream title;
   title<<config_.title<<" | "<<std::fixed<<std::setprecision(0)<<displayedFps_<<" FPS"
    <<" | chunks "<<s.renderedChunks<<"/"<<s.loadedChunks
    <<" | quads "<<s.opaqueQuads<<"+"<<s.transparentQuads
    <<" | verts "<<s.gpuVertices
    <<" | tris "<<s.gpuTriangles;
   SDL_SetWindowTitle(window_,title.str().c_str());
  }
 }
 return 0;
}
}
