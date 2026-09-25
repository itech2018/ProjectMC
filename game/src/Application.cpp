#include "projectmc/game/Application.hpp"
#include "projectmc/Log.hpp"
#include <chrono>
#include <utility>
namespace projectmc::game {
Application::Application(projectmc::GameConfig config) : config_(std::move(config)) {}
Application::~Application(){if(renderer_)SDL_DestroyRenderer(renderer_);if(window_)SDL_DestroyWindow(window_);SDL_Quit();}
bool Application::initialize(){
 if(!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS)){projectmc::log(projectmc::LogLevel::Error,SDL_GetError());return false;}
 SDL_WindowFlags flags=SDL_WINDOW_RESIZABLE;if(config_.fullscreen)flags|=SDL_WINDOW_FULLSCREEN;
 window_=SDL_CreateWindow(config_.title.c_str(),config_.width,config_.height,flags);if(!window_){projectmc::log(projectmc::LogLevel::Error,SDL_GetError());return false;}
 renderer_=SDL_CreateRenderer(window_,nullptr);if(!renderer_){projectmc::log(projectmc::LogLevel::Error,SDL_GetError());return false;}
 SDL_SetRenderVSync(renderer_,config_.vsync?1:0);world_.generateTestWorld();running_=true;
 projectmc::log(projectmc::LogLevel::Info,"Voxel test world initialized.");return true;
}
void Application::processEvents(){
 SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_EVENT_QUIT)running_=false;if(e.type==SDL_EVENT_KEY_DOWN&&e.key.key==SDLK_ESCAPE)running_=false;}
 const bool*k=SDL_GetKeyboardState(nullptr);input_.forward=k[SDL_SCANCODE_W];input_.backward=k[SDL_SCANCODE_S];input_.left=k[SDL_SCANCODE_A];input_.right=k[SDL_SCANCODE_D];input_.jump=k[SDL_SCANCODE_SPACE];
}
void Application::update(double dt){
 const float s=8.0f*static_cast<float>(dt);
 if(input_.forward)camera_.position.z-=s;if(input_.backward)camera_.position.z+=s;
 if(input_.left)camera_.position.x-=s;if(input_.right)camera_.position.x+=s;
 if(input_.jump)camera_.position.y+=s;
}
void Application::render(){
 SDL_SetRenderDrawColor(renderer_,105,175,230,255);SDL_RenderClear(renderer_);
 int w=0,h=0;SDL_GetRenderOutputSize(renderer_,&w,&h);
 if(const auto*c=world_.findChunk({0,0,0}))chunkRenderer_.render(renderer_,*c,world_.blocks(),camera_,w,h);
 SDL_RenderPresent(renderer_);
}
int Application::run(){using clock=std::chrono::steady_clock;auto previous=clock::now();while(running_){const auto now=clock::now();const std::chrono::duration<double>elapsed=now-previous;previous=now;processEvents();update(elapsed.count());render();}return 0;}
}
