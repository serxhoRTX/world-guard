#include  "Guard.h"

std::unordered_map<std::string , Guard::Region>Guard::Regions;
bool Guard::processOngoing = false , Guard::oneOutOfTwo = false;
std::string Guard::requestingPlayer ,Guard::newRegionName , Guard::configPath = "plugins/worldguard/config.yaml" , Guard::delRegion;




ENDSTONE_PLUGIN("world_guard" , "0.1.0" , Guard){
    description = "world_guard";

    command("worldguard")
    .description("Brings up the WorldGuard ui")
    .usages("/worldguard")
    .aliases("wg")
    .permissions("world_guard.command.worldguard");

    permission("world_guard.command.word_guard")
    .description("Allow user to use /world_guard command")
    .default_(endstone::PermissionDefault::Operator);







}

