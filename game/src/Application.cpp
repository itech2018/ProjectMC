#include "projectmc/game/Application.hpp"
#include "projectmc/Log.hpp"
#include <chrono>
#include <cmath>
#include <utility>
namespace projectmc::game {
constexpr float PI=3.14159265358979323846f;
Application::Application(projectmc::GameConfig c):config_(std::move(c)){}
Application::~Application(){if(renderer_)SDL_DestroyRenderer(renderer_);if(window_)SDL_DestroyWindow(window_);SDL_Quit();}
bool Application::initialize(){
 if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)){projectmc::log(projectmc::LogLevel::Error,SDL_GetError());return false;}
 SDL_WindowFlags f=SDL_WINDOW_RESIZABLE;if(config_.fullscreen)f|=SDL_WINDOW_FULLSCREEN;
 window_=SDL_CreateWindow(config_.title.c_str(),config_.width,config_.height,f);if(!window_)return false;
 renderer_=SDL_CreateRenderer(window_,nullptr);if(!renderer_)return false;SDL_SetRenderVSync(renderer_,config_.vsync?1:0);if(!atlas_.create(renderer_)){projectmc::log(projectmc::LogLevel::Error,"Failed to create block texture atlas");return false;}
 SDL_SetWindowRelativeMouseMode(window_,true);world_.generateTestWorld();running_=true;return true;
}
void Application::processEvents(){
 input_.mouseDeltaX=input_.mouseDeltaY=0;input_.removeBlock=input_.placeBlock=false;input_.hotbarSelection=-1;
 SDL_Event e;while(SDL_PollEvent(&e)){
  if(e.type==SDL_EVENT_QUIT)running_=false;
  if(e.type==SDL_EVENT_KEY_DOWN&&e.key.key==SDLK_ESCAPE)running_=false;
  if(e.type==SDL_EVENT_MOUSE_MOTION){input_.mouseDeltaX+=e.motion.xrel;input_.mouseDeltaY+=e.motion.yrel;}
  if(e.type==SDL_EVENT_MOUSE_BUTTON_DOWN){if(e.button.button==SDL_BUTTON_LEFT)input_.removeBlock=true;if(e.button.button==SDL_BUTTON_RIGHT)input_.placeBlock=true;} if(e.type==SDL_EVENT_KEY_DOWN&&e.key.key>=SDLK_1&&e.key.key<=SDLK_5)input_.hotbarSelection=(int)(e.key.key-SDLK_1);
 }
 const bool*k=SDL_GetKeyboardState(nullptr);input_.forward=k[SDL_SCANCODE_W];input_.backward=k[SDL_SCANCODE_S];input_.left=k[SDL_SCANCODE_A];input_.right=k[SDL_SCANCODE_D];input_.jump=k[SDL_SCANCODE_SPACE];input_.descend=k[SDL_SCANCODE_LCTRL];input_.sprint=k[SDL_SCANCODE_LSHIFT];
}
void Application::interact(bool place){auto h=raycastBlocks(world_,camera_);if(!h.hit)return;if(place)world_.setBlock(h.previousX,h.previousY,h.previousZ,selectedBlock_);else world_.setBlock(h.x,h.y,h.z,0);}
void Application::update(double dt){camera_.yaw+=input_.mouseDeltaX*.12f;camera_.pitch-=input_.mouseDeltaY*.12f;if(camera_.pitch>89)camera_.pitch=89;if(camera_.pitch<-89)camera_.pitch=-89;player_.update(dt,input_,world_,camera_.yaw);camera_.position=player_.eyePosition();if(input_.hotbarSelection>=0){selectedSlot_=input_.hotbarSelection;const char* names[5]={"stone","dirt","grass","sand","water"};selectedBlock_=world_.blocks().id(names[selectedSlot_]);}if(input_.removeBlock)interact(false);if(input_.placeBlock)interact(true);}
void Application::render(){SDL_SetRenderDrawColor(renderer_,105,175,230,255);SDL_RenderClear(renderer_);int w=0,h=0;SDL_GetRenderOutputSize(renderer_,&w,&h);if(auto*ch=world_.findChunk({0,0,0}))chunkRenderer_.render(renderer_,*ch,world_.blocks(),atlas_,camera_,w,h);auto hit=raycastBlocks(world_,camera_);if(hit.hit)chunkRenderer_.renderSelection(renderer_,hit.x,hit.y,hit.z,camera_,w,h);const float slot=42,gap=4,total=5*slot+4*gap,start=w*.5f-total*.5f,y=h-58;for(int i=0;i<5;++i){SDL_FRect box{start+i*(slot+gap),y,slot,slot};if(i==selectedSlot_)SDL_SetRenderDrawColor(renderer_,255,255,255,255);else SDL_SetRenderDrawColor(renderer_,70,70,70,220);SDL_RenderRect(renderer_,&box);SDL_FRect inner{box.x+5,box.y+5,box.w-10,box.h-10};switch(i){case 0:SDL_SetRenderDrawColor(renderer_,120,120,125,255);break;case 1:SDL_SetRenderDrawColor(renderer_,120,80,50,255);break;case 2:SDL_SetRenderDrawColor(renderer_,90,155,70,255);break;case 3:SDL_SetRenderDrawColor(renderer_,210,195,135,255);break;default:SDL_SetRenderDrawColor(renderer_,60,120,210,255);}SDL_RenderFillRect(renderer_,&inner);}SDL_SetRenderDrawColor(renderer_,255,255,255,255);SDL_RenderLine(renderer_,w/2-6,h/2,w/2+6,h/2);SDL_RenderLine(renderer_,w/2,h/2-6,w/2,h/2+6);SDL_RenderPresent(renderer_);}
int Application::run(){using C=std::chrono::steady_clock;auto p=C::now();while(running_){auto n=C::now();std::chrono::duration<double>d=n-p;p=n;processEvents();update(d.count());render();}return 0;}
}
