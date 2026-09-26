#include "projectmc/game/Application.hpp"
#include "projectmc/Log.hpp"
#include "projectmc/game/SdlRenderBackend.hpp"
#include "projectmc/game/GpuRenderBackend.hpp"
#include "projectmc/world/WorldMetadata.hpp"
#include "projectmc/world/WorldManager.hpp"
#include <chrono>
#include <cmath>
#include <utility>
#include <sstream>
#include <iomanip>
namespace projectmc::game {
constexpr float PI=3.14159265358979323846f;
Application::Application(projectmc::GameConfig c):config_(std::move(c)){}
Application::~Application(){
 saveCurrentWorld();
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
 projectmc::log(projectmc::LogLevel::Info,"Discovering worlds...");
 world::WorldManager worldManager("saves");
 auto selectedWorld=worldManager.ensureDefaultWorld();
 availableWorlds_=worldManager.listWorlds();
 if(availableWorlds_.empty())availableWorlds_.push_back(selectedWorld);
 worldPath_=selectedWorld.path;worldName_=selectedWorld.metadata.name;
 projectmc::log(projectmc::LogLevel::Info,"Loading world: "+worldName_+" ["+selectedWorld.id+"].");
 world::WorldMetadata metadata;
 if(world::loadWorldMetadata((worldPath_/"level.meta").string(),metadata)) {
  world_.setSeed(metadata.seed);
  player_.position={metadata.playerX,metadata.playerY,metadata.playerZ};
  camera_.yaw=metadata.yaw;camera_.pitch=metadata.pitch;
  projectmc::log(projectmc::LogLevel::Info,"Loaded world metadata: "+metadata.name+" (seed "+std::to_string(metadata.seed)+").");
 } else {
  world_.setSeed(metadata.seed);
  projectmc::log(projectmc::LogLevel::Warning,"Selected world metadata could not be loaded; using defaults.");
  metadata.name=worldName_;
  world::saveWorldMetadata((worldPath_/"level.meta").string(),metadata);
 }
 if(world_.loadOverrides((worldPath_/"world.pmc").string()))
  projectmc::log(projectmc::LogLevel::Info,"Loaded "+std::to_string(world_.overrideCount())+" saved block override(s).");
 else
  projectmc::log(projectmc::LogLevel::Info,"No existing development save found; starting fresh.");
 projectmc::log(projectmc::LogLevel::Info,"Generating spawn terrain...");
 world_.generateTerrain(config_.viewDistance);
 // Saved positions can be invalid after falling out of the world. Recover to the
 // highest solid block near the saved X/Z (or the default spawn column).
 auto recoverPlayer=[&](){
  auto findSafeY=[&](int x,int z)->float{
   for(int y=world::Chunk::Height-1;y>=0;--y){
    const auto id=world_.getBlock(x,y,z);
    if(id!=0&&world_.blocks().get(id).solid)return static_cast<float>(y+1);
   }
   return -1.0f;
  };
  const bool finite=std::isfinite(player_.position.x)&&std::isfinite(player_.position.y)&&std::isfinite(player_.position.z);
  int x=finite?static_cast<int>(std::floor(player_.position.x)):8;
  int z=finite?static_cast<int>(std::floor(player_.position.z)):12;
  // Ensure the saved column exists before deciding whether the saved position is safe.
  world_.updateStreaming(static_cast<float>(x),static_cast<float>(z),config_.viewDistance);
  const float safeY=findSafeY(x,z);
  const bool verticalValid=finite&&player_.position.y>=0.0f&&player_.position.y<static_cast<float>(world::Chunk::Height+8);
  // A valid saved position must be at/above the terrain surface and close enough
  // that it cannot represent an old fall through empty space.
  const bool supported=verticalValid&&safeY>=0.0f&&player_.position.y>=safeY-0.05f&&player_.position.y<=safeY+4.0f;
  if(supported)return;
  float y=safeY;
  if(y<0.0f){x=8;z=12;y=findSafeY(x,z);}
  if(y<0.0f)y=13.0f;
  player_.position={static_cast<float>(x)+0.5f,y,static_cast<float>(z)+0.5f};
  player_.velocity={};
  camera_.yaw=0.0f;
  camera_.pitch=-20.0f;
  camera_.position=player_.eyePosition();
  projectmc::log(projectmc::LogLevel::Warning,"Recovered player position and camera from an invalid save state.");
 };
 recoverPlayer();
 // Pump the window once before grabbing the mouse. On Windows this avoids capturing
 // input while the SDL window is still being created/activated.
 SDL_PumpEvents();SDL_RaiseWindow(window_);
 SDL_SetWindowRelativeMouseMode(window_,false);
 screen_=Screen::Title;menuSelection_=0;
 SDL_SetWindowTitle(window_,"ProjectMC | Main Menu | Enter: Singleplayer | Q: Quit");
 running_=true;
 projectmc::log(projectmc::LogLevel::Info,"Game initialised - entering main loop.");
 return true;
}
void Application::saveCurrentWorld(){
 if(worldPath_.empty())return;
 world::WorldMetadata metadata;metadata.name=worldName_;metadata.seed=world_.seed();
 metadata.playerX=player_.position.x;metadata.playerY=player_.position.y;metadata.playerZ=player_.position.z;
 metadata.yaw=camera_.yaw;metadata.pitch=camera_.pitch;
 if(!world::saveWorldMetadata((worldPath_/"level.meta").string(),metadata))
  projectmc::log(projectmc::LogLevel::Warning,"Could not save world metadata: "+worldName_);
 if(!world_.saveOverrides((worldPath_/"world.pmc").string()))
  projectmc::log(projectmc::LogLevel::Warning,"Could not save world overrides: "+worldName_);
 else projectmc::log(projectmc::LogLevel::Info,"Saved world: "+worldName_+".");
}
void Application::leaveWorldToMenu(){
 saveCurrentWorld();
 if(auto* gpu=dynamic_cast<GpuRenderBackend*>(renderBackend_.get()))chunkRenderer_.releaseGpuMeshes(*gpu);
 world::WorldManager manager("saves");availableWorlds_=manager.listWorlds();
 screen_=Screen::Title;menuSelection_=0;SDL_SetWindowRelativeMouseMode(window_,false);
}
void Application::loadWorldEntry(const world::WorldEntry& entry){
 if(screen_==Screen::Playing)saveCurrentWorld();
 if(auto* gpu=dynamic_cast<GpuRenderBackend*>(renderBackend_.get()))chunkRenderer_.releaseGpuMeshes(*gpu);
 worldPath_=entry.path;worldName_=entry.metadata.name;world_.reset(entry.metadata.seed);
 world::WorldMetadata metadata=entry.metadata;
 world::loadWorldMetadata((worldPath_/"level.meta").string(),metadata);
 player_.position={metadata.playerX,metadata.playerY,metadata.playerZ};player_.velocity={};
 camera_.yaw=metadata.yaw;camera_.pitch=metadata.pitch;camera_.position=player_.eyePosition();
 world_.loadOverrides((worldPath_/"world.pmc").string());
 world_.generateTerrain(config_.viewDistance);
 screen_=Screen::Playing;SDL_SetWindowRelativeMouseMode(window_,true);
}
void Application::activateMenuSelection(){
 if(screen_==Screen::Title){if(menuSelection_==0){screen_=Screen::Singleplayer;menuSelection_=0;selectedWorld_=0;worldListOffset_=0;}else running_=false;return;}
 if(screen_==Screen::Singleplayer){
  const int n=(int)availableWorlds_.size();
  if(menuSelection_<n){selectedWorld_=menuSelection_;return;}
  const int action=menuSelection_-n;
  if(action==0&&n){loadWorldEntry(availableWorlds_[std::clamp(selectedWorld_,0,n-1)]);}
  else if(action==1){screen_=Screen::CreateWorld;createField_=0;createWorldName_="New World";createWorldSeed_.clear();SDL_StartTextInput(window_);}
  else if(action==2&&n){int i=std::clamp(selectedWorld_,0,n-1);renameWorldName_=availableWorlds_[i].metadata.name;menuSelection_=i;confirmSelection_=0;screen_=Screen::RenameWorld;SDL_StartTextInput(window_);}
  else if(action==3&&n){menuSelection_=std::clamp(selectedWorld_,0,n-1);screen_=Screen::DeleteWorld;confirmSelection_=1;}
  else {screen_=Screen::Title;menuSelection_=0;}return;
 }
 if(screen_==Screen::CreateWorld){
  if(createField_==2){std::uint64_t seed=0;if(!createWorldSeed_.empty())try{seed=std::stoull(createWorldSeed_);}catch(...){}
   if(!seed)seed=(std::uint64_t)SDL_GetTicksNS();world::WorldManager m("saves");auto entry=m.createWorld(createWorldName_.empty()?"New World":createWorldName_,seed);
   availableWorlds_=m.listWorlds();SDL_StopTextInput(window_);loadWorldEntry(entry);}
  else if(createField_==3){SDL_StopTextInput(window_);screen_=Screen::Singleplayer;}return;
 }
 if(screen_==Screen::RenameWorld){
  if(confirmSelection_==0&&!availableWorlds_.empty()){world::WorldManager m("saves");const int i=std::clamp(menuSelection_,0,(int)availableWorlds_.size()-1);m.renameWorld(availableWorlds_[i],renameWorldName_);availableWorlds_=m.listWorlds();}
  SDL_StopTextInput(window_);screen_=Screen::Singleplayer;return;
 }
 if(screen_==Screen::DeleteWorld){
  if(confirmSelection_==0&&!availableWorlds_.empty()){world::WorldManager m("saves");const int i=std::clamp(menuSelection_,0,(int)availableWorlds_.size()-1);m.deleteWorld(availableWorlds_[i]);availableWorlds_=m.listWorlds();menuSelection_=std::max(0,std::min(i,(int)availableWorlds_.size()-1));}
  screen_=Screen::Singleplayer;return;
 }
}
void Application::processEvents(){
 input_.mouseDeltaX=input_.mouseDeltaY=0;input_.removeBlock=input_.placeBlock=false;input_.hotbarSelection=-1;SDL_Event e;
 while(SDL_PollEvent(&e)){
  if(e.type==SDL_EVENT_QUIT){running_=false;continue;}if(e.type==SDL_EVENT_WINDOW_RESIZED&&renderBackend_)renderBackend_->resize(e.window.data1,e.window.data2);
  if(e.type==SDL_EVENT_WINDOW_FOCUS_GAINED&&screen_==Screen::Playing)SDL_SetWindowRelativeMouseMode(window_,true);
  if(e.type==SDL_EVENT_WINDOW_FOCUS_LOST){SDL_SetWindowRelativeMouseMode(window_,false);input_.forward=input_.backward=input_.left=input_.right=input_.jump=input_.descend=input_.sprint=false;}
  if(screen_!=Screen::Playing){
   if(e.type==SDL_EVENT_TEXT_INPUT){
    if(screen_==Screen::CreateWorld&&createField_<2){auto& f=createField_==0?createWorldName_:createWorldSeed_;std::string add=e.text.text;if(createField_==1)add.erase(std::remove_if(add.begin(),add.end(),[](unsigned char ch){return !std::isdigit(ch);}),add.end());if(f.size()+add.size()<=32)f+=add;}
    else if(screen_==Screen::RenameWorld&&renameWorldName_.size()<32)renameWorldName_+=e.text.text;
   }
   if(e.type==SDL_EVENT_MOUSE_WHEEL&&screen_==Screen::Singleplayer&&!availableWorlds_.empty()){worldListOffset_=std::clamp(worldListOffset_-(int)e.wheel.y,0,std::max(0,(int)availableWorlds_.size()-5));}
   if(e.type==SDL_EVENT_KEY_DOWN&&!e.key.repeat){
    if(screen_==Screen::CreateWorld){if(e.key.key==SDLK_TAB||e.key.key==SDLK_DOWN)createField_=(createField_+1)%4;if(e.key.key==SDLK_UP)createField_=(createField_+3)%4;if(e.key.key==SDLK_BACKSPACE&&createField_<2){auto&f=createField_==0?createWorldName_:createWorldSeed_;if(!f.empty())f.pop_back();}if(e.key.key==SDLK_RETURN||e.key.key==SDLK_KP_ENTER){if(createField_<2)++createField_;else activateMenuSelection();}if(e.key.key==SDLK_ESCAPE){SDL_StopTextInput(window_);screen_=Screen::Singleplayer;}}
    else if(screen_==Screen::RenameWorld){if(e.key.key==SDLK_BACKSPACE&&!renameWorldName_.empty())renameWorldName_.pop_back();if(e.key.key==SDLK_RETURN||e.key.key==SDLK_KP_ENTER){confirmSelection_=0;activateMenuSelection();}if(e.key.key==SDLK_ESCAPE){confirmSelection_=1;activateMenuSelection();}}
    else if(screen_==Screen::DeleteWorld){if(e.key.key==SDLK_LEFT||e.key.key==SDLK_RIGHT||e.key.key==SDLK_TAB)confirmSelection_=1-confirmSelection_;if(e.key.key==SDLK_RETURN||e.key.key==SDLK_KP_ENTER)activateMenuSelection();if(e.key.key==SDLK_ESCAPE){confirmSelection_=1;activateMenuSelection();}}
    else {const int maxItem=screen_==Screen::Title?1:(int)availableWorlds_.size()+4;if(e.key.key==SDLK_UP||e.key.key==SDLK_W)menuSelection_=std::max(0,menuSelection_-1);if(e.key.key==SDLK_DOWN||e.key.key==SDLK_S)menuSelection_=std::min(maxItem,menuSelection_+1);if(e.key.key==SDLK_RETURN||e.key.key==SDLK_KP_ENTER)activateMenuSelection();if(e.key.key==SDLK_ESCAPE){if(screen_==Screen::Singleplayer){screen_=Screen::Title;menuSelection_=0;}else running_=false;}}
   }
   if(screen_==Screen::Singleplayer&&menuSelection_<(int)availableWorlds_.size()){selectedWorld_=menuSelection_;if(selectedWorld_<worldListOffset_)worldListOffset_=selectedWorld_;if(selectedWorld_>=worldListOffset_+5)worldListOffset_=selectedWorld_-4;}
   if(e.type==SDL_EVENT_MOUSE_MOTION){int w=0,h=0;SDL_GetWindowSizeInPixels(window_,&w,&h);float nx=e.motion.x/w*2-1,ny=1-e.motion.y/h*2;
    if(screen_==Screen::Title){if(std::abs(nx)<=.30f&&ny>=.055f&&ny<=.185f)menuSelection_=0;else if(std::abs(nx)<=.30f&&ny>=-.145f&&ny<=-.015f)menuSelection_=1;}
    else if(screen_==Screen::Singleplayer){for(int row=0;row<5&&worldListOffset_+row<(int)availableWorlds_.size();++row){float y=.31f-row*.13f;if(std::abs(nx)<=.48f&&ny>=y-.05f&&ny<=y+.05f){menuSelection_=worldListOffset_+row;selectedWorld_=menuSelection_;}}}
   }
   if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN&&e.button.button==SDL_BUTTON_LEFT){
    int w=0,h=0;SDL_GetWindowSizeInPixels(window_,&w,&h);float nx=e.button.x/w*2-1,ny=1-e.button.y/h*2;bool hit=false;
    if(screen_==Screen::Title){if(std::abs(nx)<=.32f&&ny>=.055f&&ny<=.185f){menuSelection_=0;hit=true;}else if(std::abs(nx)<=.32f&&ny>=-.145f&&ny<=-.015f){menuSelection_=1;hit=true;}}
    else if(screen_==Screen::Singleplayer){int n=(int)availableWorlds_.size();for(int row=0;row<5&&worldListOffset_+row<n;++row){float y=.31f-row*.13f;if(std::abs(nx)<=.48f&&ny>=y-.05f&&ny<=y+.05f){menuSelection_=worldListOffset_+row;selectedWorld_=menuSelection_;hit=true;}}
     if(nx>=-.19f&&nx<=.19f&&ny>=-.355f&&ny<=-.265f){menuSelection_=n;hit=true;}
     else if(nx>=-.44f&&nx<=-.06f&&ny>=-.475f&&ny<=-.385f){menuSelection_=n+1;hit=true;}
     else if(nx>=.06f&&nx<=.44f&&ny>=-.475f&&ny<=-.385f){menuSelection_=n+2;hit=true;}
     else if(nx>=-.44f&&nx<=-.06f&&ny>=-.595f&&ny<=-.505f){menuSelection_=n+3;hit=true;}
     else if(nx>=.06f&&nx<=.44f&&ny>=-.595f&&ny<=-.505f){menuSelection_=n+4;hit=true;}
    }else if(screen_==Screen::CreateWorld){if(std::abs(nx)<=.44f&&ny>=.145f&&ny<=.255f){createField_=0;hit=true;}else if(std::abs(nx)<=.44f&&ny>=-.015f&&ny<=.095f){createField_=1;hit=true;}else if(nx<0&&ny>=-.535f&&ny<=-.425f){createField_=2;hit=true;}else if(nx>=0&&ny>=-.535f&&ny<=-.425f){createField_=3;hit=true;}}
    else if(screen_==Screen::RenameWorld){if(std::abs(nx)<=.44f&&ny>=.105f&&ny<=.215f){hit=true;}else if(ny>=-.535f&&ny<=-.425f){confirmSelection_=nx<0?0:1;hit=true;}}
    else if(screen_==Screen::DeleteWorld&&ny>=-.535f&&ny<=-.425f){confirmSelection_=nx<0?0:1;hit=true;}
    if(hit&&!(screen_==Screen::RenameWorld&&std::abs(nx)<=.44f&&ny>=.105f&&ny<=.215f))activateMenuSelection();
   }continue;
  }
  if(e.type==SDL_EVENT_KEY_DOWN||e.type==SDL_EVENT_KEY_UP){bool down=e.type==SDL_EVENT_KEY_DOWN;switch(e.key.scancode){case SDL_SCANCODE_W:input_.forward=down;break;case SDL_SCANCODE_S:input_.backward=down;break;case SDL_SCANCODE_A:input_.left=down;break;case SDL_SCANCODE_D:input_.right=down;break;case SDL_SCANCODE_SPACE:input_.jump=down;break;case SDL_SCANCODE_LCTRL:case SDL_SCANCODE_RCTRL:input_.descend=down;break;case SDL_SCANCODE_LSHIFT:case SDL_SCANCODE_RSHIFT:input_.sprint=down;break;default:break;}if(down&&!e.key.repeat){if(e.key.key==SDLK_ESCAPE)leaveWorldToMenu();if(e.key.key>=SDLK_1&&e.key.key<=SDLK_5)input_.hotbarSelection=(int)(e.key.key-SDLK_1);}}
  if(e.type==SDL_EVENT_MOUSE_MOTION){input_.mouseDeltaX+=e.motion.xrel;input_.mouseDeltaY+=e.motion.yrel;}if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN){if(e.button.button==SDL_BUTTON_LEFT)input_.removeBlock=true;if(e.button.button==SDL_BUTTON_RIGHT)input_.placeBlock=true;}
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
void Application::update(double dt){if(screen_!=Screen::Playing)return;constexpr float mouseSensitivity=.09f;
 camera_.yaw+=input_.mouseDeltaX*mouseSensitivity;
 camera_.pitch-=input_.mouseDeltaY*mouseSensitivity;
 if(camera_.yaw>180.0f)camera_.yaw-=360.0f;
 if(camera_.yaw<-180.0f)camera_.yaw+=360.0f;if(camera_.pitch>89)camera_.pitch=89;if(camera_.pitch<-89)camera_.pitch=-89;player_.update(dt,input_,world_,camera_.yaw);camera_.position=player_.eyePosition();world_.updateStreaming(player_.position.x,player_.position.z,config_.viewDistance);if(input_.hotbarSelection>=0){selectedSlot_=input_.hotbarSelection;const char* names[5]={"stone","dirt","grass","sand","water"};selectedBlock_=world_.blocks().id(names[selectedSlot_]);}if(input_.removeBlock)interact(false);if(input_.placeBlock)interact(true);}
void Application::render(){
 if(gpuMode_&&screen_!=Screen::Playing){
  auto* gpu=dynamic_cast<GpuRenderBackend*>(renderBackend_.get());if(!gpu)return;
  gpu->beginFrame(camera_);
  std::string menuWorld;
  if(screen_==Screen::Singleplayer&&!availableWorlds_.empty()){
   const int i=std::clamp(menuSelection_,0,(int)availableWorlds_.size()-1);
   menuWorld=availableWorlds_[i].metadata.name;
  }
  const int menuScreen=screen_==Screen::Title?0:screen_==Screen::Singleplayer?1:screen_==Screen::CreateWorld?2:screen_==Screen::RenameWorld?3:4;
  const int selected=screen_==Screen::CreateWorld?createField_:(screen_==Screen::RenameWorld||screen_==Screen::DeleteWorld?confirmSelection_:menuSelection_);
  const std::string shownName=screen_==Screen::CreateWorld?createWorldName_:screen_==Screen::RenameWorld?renameWorldName_:menuWorld;
  gpu->drawMenu(menuScreen,selected,screen_==Screen::Title?2:(int)availableWorlds_.size(),shownName,
   screen_==Screen::CreateWorld?createWorldSeed_:"",worldListOffset_);
  gpu->endFrame();return;
 }
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
   if(screen_==Screen::Playing)SDL_SetWindowTitle(window_,title.str().c_str());
  }
 }
 return 0;
}
}
