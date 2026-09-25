#include "projectmc/world/BlockRegistry.hpp"
#include <stdexcept>
namespace projectmc::world {
BlockRegistry::BlockRegistry(){
 registerBlock("air",BlockMaterial::Air,false,true);
 registerBlock("stone",BlockMaterial::Stone,true,false,{"stone","stone","stone"});
 registerBlock("dirt",BlockMaterial::Soil,true,false,{"dirt","dirt","dirt"});
 registerBlock("grass",BlockMaterial::Soil,true,false,{"grass_top","grass_side","dirt"});
 registerBlock("sand",BlockMaterial::Sand,true,false,{"sand","sand","sand"});
 registerBlock("water",BlockMaterial::Water,false,true,{"water","water","water"});
}
BlockId BlockRegistry::registerBlock(std::string name,BlockMaterial material,bool solid,bool transparent,BlockTextures textures){
 if(names_.contains(name))throw std::runtime_error("Duplicate block: "+name);
 if(blocks_.size()>=65536)throw std::runtime_error("Block registry exhausted");
 auto value=static_cast<BlockId>(blocks_.size());names_.emplace(name,value);
 blocks_.push_back({value,std::move(name),material,solid,transparent,std::move(textures)});return value;
}
const BlockDefinition& BlockRegistry::get(BlockId id)const{if(id>=blocks_.size())throw std::out_of_range("Unknown block id");return blocks_[id];}
const BlockDefinition& BlockRegistry::get(std::string_view name)const{return get(id(name));}
BlockId BlockRegistry::id(std::string_view name)const{auto it=names_.find(std::string(name));if(it==names_.end())throw std::out_of_range("Unknown block name");return it->second;}
}
