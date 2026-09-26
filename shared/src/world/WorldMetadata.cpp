#include "projectmc/world/WorldMetadata.hpp"
#include <filesystem>
#include <fstream>
#include <iomanip>

namespace projectmc::world {
bool saveWorldMetadata(const std::string& path,const WorldMetadata& m) {
 std::error_code ec;const auto parent=std::filesystem::path(path).parent_path();
 if(!parent.empty())std::filesystem::create_directories(parent,ec);
 std::ofstream out(path,std::ios::trunc);if(!out)return false;
 out<<"format_version="<<m.formatVersion<<"\n";
 out<<"name="<<std::quoted(m.name)<<"\n";
 out<<"seed="<<m.seed<<"\n";
 out<<std::setprecision(9);
 out<<"player_x="<<m.playerX<<"\nplayer_y="<<m.playerY<<"\nplayer_z="<<m.playerZ<<"\n";
 out<<"yaw="<<m.yaw<<"\npitch="<<m.pitch<<"\n";
 return static_cast<bool>(out);
}
bool loadWorldMetadata(const std::string& path,WorldMetadata& m) {
 std::ifstream in(path);if(!in)return false;
 std::string line;
 while(std::getline(in,line)){
  const auto p=line.find('=');if(p==std::string::npos)continue;
  const auto key=line.substr(0,p),value=line.substr(p+1);
  try {
   if(key=="format_version")m.formatVersion=static_cast<std::uint32_t>(std::stoul(value));
   else if(key=="name"){std::istringstream s(value);s>>std::quoted(m.name);}
   else if(key=="seed")m.seed=std::stoull(value);
   else if(key=="player_x")m.playerX=std::stof(value);
   else if(key=="player_y")m.playerY=std::stof(value);
   else if(key=="player_z")m.playerZ=std::stof(value);
   else if(key=="yaw")m.yaw=std::stof(value);
   else if(key=="pitch")m.pitch=std::stof(value);
  } catch(...) {return false;}
 }
 return m.formatVersion==1;
}
}
