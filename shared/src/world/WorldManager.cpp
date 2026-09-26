#include "projectmc/world/WorldManager.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <system_error>

namespace projectmc::world {
std::string WorldManager::safeWorldId(const std::string& name){
 std::string out;bool dash=false;
 for(unsigned char ch:name){
  if(std::isalnum(ch)){out.push_back(static_cast<char>(std::tolower(ch)));dash=false;}
  else if(!out.empty()&&!dash){out.push_back('-');dash=true;}
 }
 while(!out.empty()&&out.back()=='-')out.pop_back();
 return out.empty()?"world":out;
}
std::vector<WorldEntry> WorldManager::listWorlds()const{
 std::vector<WorldEntry> worlds;std::error_code ec;
 if(!std::filesystem::exists(root_,ec))return worlds;
 for(const auto& item:std::filesystem::directory_iterator(root_,ec)){
  if(ec)break;if(!item.is_directory())continue;
  WorldMetadata metadata;
  if(loadWorldMetadata((item.path()/"level.meta").string(),metadata))
   worlds.push_back({item.path().filename().string(),item.path(),std::move(metadata)});
 }
 std::sort(worlds.begin(),worlds.end(),[](const auto&a,const auto&b){return a.metadata.name<b.metadata.name;});
 return worlds;
}
WorldEntry WorldManager::createWorld(const std::string& name,std::uint64_t seed)const{
 std::error_code ec;std::filesystem::create_directories(root_,ec);
 const std::string base=safeWorldId(name);std::string id=base;int suffix=2;
 while(std::filesystem::exists(root_/id,ec))id=base+"-"+std::to_string(suffix++);
 const auto path=root_/id;std::filesystem::create_directories(path,ec);
 WorldMetadata metadata;metadata.name=name.empty()?"New World":name;metadata.seed=seed;
 saveWorldMetadata((path/"level.meta").string(),metadata);
 return {id,path,metadata};
}
WorldEntry WorldManager::ensureDefaultWorld()const{
 auto worlds=listWorlds();if(!worlds.empty())return worlds.front();
 const auto seed=static_cast<std::uint64_t>(std::chrono::system_clock::now().time_since_epoch().count());
 return createWorld("Development World",seed);
}
}
