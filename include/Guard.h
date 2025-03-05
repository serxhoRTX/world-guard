#pragma once
#include <endstone/plugin/plugin.h>
#include <endstone/event/block/block_break_event.h>
#include <endstone/color_format.h>
#include <endstone/event/player/player_chat_event.h>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <endstone/form/form.h>
#include <endstone/form/controls/text_input.h>
#include <endstone/form/controls/toggle.h>
#include <endstone/event/block/block_place_event.h>


class Guard : public endstone::Plugin
{
private:
    struct Region
    {
        struct Boundaries
        {
            int x1, y1, z1, x2, y2, z2;
        } boundaries;

        struct Permissions
        {
            bool canPlace, canBreak, canPvp, canTakeDamage;
        } permissions;
    };

    static std::unordered_map<std::string, Region> Regions;
    Region settup_region_;
    static bool processOngoing, oneOutOfTwo;
    static std::string requestingPlayer, newRegionName, configPath, delRegion;
    std::string wg = endstone::ColorFormat::Red + endstone::ColorFormat::Bold + "[WorldGuard] " +
        endstone::ColorFormat::Reset;

public:
    void onLoad() override { getLogger().info("World Guard was loaded !"); }
    void onDisable() override { getLogger().info("World Guard was disabled !"); }

    void onEnable() override
    {
        getLogger().info("WorldGuard was enabled");
        registerEvent(&Guard::onBreakEvent, *this);
        readRegionsFromFile(configPath);
        registerEvent(&Guard::onPlaceEvent, *this);
    }

    bool onCommand(endstone::CommandSender& sender, const endstone::Command& command,
                   const std::vector<std::string>& args) override
    {
        if (command.getName() == "worldguard")
        {
            if (sender.asConsole())
            {
                sender.sendMessage("This command is not for console !");
                return true;
            }
            if (auto player = sender.asPlayer())
            {
                createWorldGuardForm(*player);
            }
        }

        return true;
    }


    void onBreakEvent(endstone::BlockBreakEvent& event)
    {
        if (locatedInsideRegion(event.getBlock().getLocation(), 0))
        {
            event.cancel();
            return;
        }


        if (processOngoing && event.getPlayer().getName() == requestingPlayer)
        {
            settup_region_.boundaries.x1 = event.getBlock().getX();
            settup_region_.boundaries.y1 = event.getBlock().getY();
            settup_region_.boundaries.z1 = event.getBlock().getZ();
            processOngoing = false;
            oneOutOfTwo = true;
            event.getPlayer().sendMessage(wg + "position 1 selected");
            event.cancel();
        }
        else if (oneOutOfTwo && event.getPlayer().getName() == requestingPlayer)
        {
            settup_region_.boundaries.x2 = event.getBlock().getX();
            settup_region_.boundaries.y2 = event.getBlock().getY();
            settup_region_.boundaries.z2 = event.getBlock().getZ();
            oneOutOfTwo = false;
            event.getPlayer().sendMessage(wg + "position 2 selected");
            event.cancel();

            nameForm(event.getPlayer());
        }
    }

    static void startSettup(endstone::Player& player)
    {
        requestingPlayer = player.getName();
        processOngoing = true;
    }

    void onPlaceEvent(endstone::BlockPlaceEvent& event)
    {
        if (locatedInsideRegion(event.getBlock().getLocation(), 1))
            event.cancel();
    }


    void editRegionForm(endstone::Player& player)
    {
        endstone::ActionForm editRegion;
        editRegion.setTitle(endstone::ColorFormat::Bold + "Select A Region");

        for (const auto& it : Regions)
        {
            editRegion.addButton(it.first, "textures/ui/welcome", [=](endstone::Player* player)
            {
                innerEditForm(*player, it.first);
            });
        }

        editRegion.addButton(endstone::Message(endstone::ColorFormat::Bold + "back"), "textures/map/village_plains",
                             [=](endstone::Player* player)
                             {
                                 createWorldGuardForm(*player);
                             });

        player.sendForm(editRegion);
    }

    void innerEditForm(endstone::Player& player, const std::string& region_name)
    {
        std::string cs = endstone::ColorFormat::MaterialRedstone + "coming soon" + endstone::ColorFormat::Reset;
        endstone::ModalForm modal_form;
        modal_form.setTitle(endstone::ColorFormat::Bold + "Edit Permissions");

        endstone::Toggle toggle_canPlace, toggle_canBreak, toggle_canPvp, toggle_canTakeDamage;
        toggle_canPlace.setLabel(endstone::ColorFormat::MaterialIron + "Allow placing blocks").setDefaultValue(
            Regions[region_name].permissions.canPlace);
        toggle_canBreak.setLabel(endstone::ColorFormat::MaterialIron + "Allow breaking blocks").setDefaultValue(
            Regions[region_name].permissions.canBreak);
        toggle_canPvp.setLabel(
                          endstone::ColorFormat::MaterialIron + "Allow pvp [" + cs + endstone::ColorFormat::MaterialIron
                          + "]").
                      setDefaultValue(Regions[region_name].permissions.canPvp);
        toggle_canTakeDamage.setLabel(
                                 endstone::ColorFormat::MaterialIron + "Take damage [" + cs +
                                 endstone::ColorFormat::MaterialIron + "]").
                             setDefaultValue(Regions[region_name].permissions.canTakeDamage);

        modal_form.addControl(toggle_canBreak).addControl(toggle_canPlace).addControl(toggle_canPvp).addControl(
            toggle_canTakeDamage);

        modal_form.setOnSubmit([=](endstone::Player* player, std::string response)
        {
            response = response.substr(1, response.length() - 2);

            std::stringstream ss(response);
            std::string value;
            int index = 0;

            while (std::getline(ss, value, ','))
            {
                value.erase(std::remove(value.begin(), value.end(), ' '), value.end());

                switch (index)
                {
                case 0:
                    Regions[region_name].permissions.canPlace = (value == "true");
                    break;
                case 1:
                    Regions[region_name].permissions.canBreak = (value == "true");
                    break;
                case 2:
                    Regions[region_name].permissions.canPvp = (value == "true");
                    break;
                case 3:
                    Regions[region_name].permissions.canTakeDamage = (value == "true");
                    break;
                default:
                    break;
                }
                ++index;
            }
            updateRegionPermissionsInFile(configPath, region_name, Regions[region_name]);
            player->sendMessage(wg + "changes saved !");
        });

        player.sendForm(modal_form);
    }


    void createWorldGuardForm(endstone::Player& player)
    {
        endstone::ActionForm WG_form;

        WG_form.setTitle(endstone::ColorFormat::Bold + "WorldGuard")
               .addButton(
                   endstone::Message(endstone::ColorFormat::Bold + "Create" + endstone::ColorFormat::Reset + " region"),
                   "textures/map/map_background", [=](endstone::Player* player)
                   {
                       endstone::ActionForm settup_form;
                       settup_form.setTitle(endstone::Message(endstone::ColorFormat::Bold + "New Region Setup"))
                                  .setContent(
                                      endstone::ColorFormat::MaterialIron +
                                      "Define your new region by selecting 2 positions!\n\nSimply break a block at each corner to set the boundaries\n")
                                  .addButton(endstone::Message(endstone::ColorFormat::Bold + "create"),
                                             "textures/ui/cartography_table_zoom", [=](endstone::Player* player)
                                             {
                                                 startSettup(*player);
                                                 player->sendMessage(wg + "break a block to select a position");
                                             })
                                  .addButton(endstone::Message(endstone::ColorFormat::Bold + "back"),
                                             "textures/map/village_plains", [=](endstone::Player* player)
                                             {
                                                 createWorldGuardForm(*player);
                                             });

                       player->sendForm(settup_form);
                   })
               .addButton(
                   endstone::Message(endstone::ColorFormat::Bold + "Edit" + endstone::ColorFormat::Reset + " region"),
                   "textures/ui/book_edit_default", [=](endstone::Player* player)
                   {
                       editRegionForm(*player);
                   })
               .addButton(
                   endstone::Message(
                       endstone::ColorFormat::Red + endstone::ColorFormat::Bold + "Delete" +
                       endstone::ColorFormat::Reset + " region"), "textures/ui/book_trash_default",
                   [=](endstone::Player* player)
                   {
                       deleteRegionForm(*player);
                   });

        player.sendForm(WG_form);
    }

    void readRegionsFromFile(const std::string& filename)
    {
        YAML::Node config = YAML::LoadFile(filename);

        if (config["Regions"])
        {
            for (const auto& regionNode : config["Regions"])
            {
                std::string regionName = regionNode.first.as<std::string>();

                Region region;
                region.boundaries.x1 = regionNode.second["boundaries"]["x1"].as<int>();
                region.boundaries.y1 = regionNode.second["boundaries"]["y1"].as<int>();
                region.boundaries.z1 = regionNode.second["boundaries"]["z1"].as<int>();
                region.boundaries.x2 = regionNode.second["boundaries"]["x2"].as<int>();
                region.boundaries.y2 = regionNode.second["boundaries"]["y2"].as<int>();
                region.boundaries.z2 = regionNode.second["boundaries"]["z2"].as<int>();

                region.permissions.canPlace = regionNode.second["permissions"]["canPlace"].as<bool>();
                region.permissions.canBreak = regionNode.second["permissions"]["canBreak"].as<bool>();
                region.permissions.canPvp = regionNode.second["permissions"]["canPvp"].as<bool>();
                region.permissions.canTakeDamage = regionNode.second["permissions"]["canTakeDamage"].as<bool>();

                Regions[regionName] = region;
                getLogger().info("Loaded region: " + regionName);
            }
        }
        else
        {
            getLogger().warning("No regions found in the YAML file!");
        }
    }

    void writeRegionToFile(const std::string& filename, const std::string& regionName, const Region& region)
    {
        YAML::Node config = YAML::LoadFile(filename);

        if (config["Regions"][regionName])
        {
            getLogger().warning("Region " + regionName + " already exists. Region not overwritten.");
            return;
        }

        YAML::Node newRegion;
        newRegion["boundaries"]["x1"] = region.boundaries.x1;
        newRegion["boundaries"]["y1"] = region.boundaries.y1;
        newRegion["boundaries"]["z1"] = region.boundaries.z1;
        newRegion["boundaries"]["x2"] = region.boundaries.x2;
        newRegion["boundaries"]["y2"] = region.boundaries.y2;
        newRegion["boundaries"]["z2"] = region.boundaries.z2;

        newRegion["permissions"]["canPlace"] = region.permissions.canPlace;
        newRegion["permissions"]["canBreak"] = region.permissions.canBreak;
        newRegion["permissions"]["canPvp"] = region.permissions.canPvp;
        newRegion["permissions"]["canTakeDamage"] = region.permissions.canTakeDamage;

        config["Regions"][regionName] = newRegion;

        std::ofstream fout(filename);
        fout << config;

        getLogger().info("Region " + regionName + " has been added to the file.");
    }

    void updateRegionPermissionsInFile(const std::string& filename, const std::string& regionName, const Region& region)
    {
        YAML::Node config = YAML::LoadFile(filename);

        if (config["Regions"][regionName])
        {
            config["Regions"][regionName]["permissions"]["canPlace"] = region.permissions.canPlace;
            config["Regions"][regionName]["permissions"]["canBreak"] = region.permissions.canBreak;
            config["Regions"][regionName]["permissions"]["canPvp"] = region.permissions.canPvp;
            config["Regions"][regionName]["permissions"]["canTakeDamage"] = region.permissions.canTakeDamage;

            std::ofstream fout(filename);
            fout << config;

            getLogger().info("Permissions for region " + regionName + " have been updated.");
        }
        else
        {
            getLogger().warning("Region " + regionName + " does not exist. Permissions not updated.");
        }
    }

    void deleteRegionFromFile(const std::string& filename, const std::string& regionName)
    {
        YAML::Node config = YAML::LoadFile(filename);
        if (config["Regions"][regionName])
        {
            config["Regions"].remove(regionName);
            std::ofstream fout(filename);
            fout << config;

            getLogger().info("Region " + regionName + " has been deleted from the file.");
        }
        else
        {
            getLogger().warning("Region " + regionName + " does not exist. Deletion not performed.");
        }
    }

    void nameForm(endstone::Player& player)
    {
        endstone::ModalForm modal_form;
        modal_form.setTitle(endstone::ColorFormat::Bold + "Enter a name for this region");

        endstone::TextInput input;
        input.setLabel("Region Name");

        std::string cs = endstone::ColorFormat::MaterialRedstone + "coming soon" + endstone::ColorFormat::Reset;
        endstone::Toggle toggle_canPlace, toggle_canBreak, toggle_canPvp, toggle_canTakeDamage;
        toggle_canPlace.setLabel("Allow placing blocks").setDefaultValue(false);
        toggle_canBreak.setLabel("Allow breaking blocks").setDefaultValue(false);
        toggle_canPvp.setLabel("Allow pvp [" + cs + "]").setDefaultValue(false);
        toggle_canTakeDamage.setLabel("Take damage [" + cs + "]").setDefaultValue(false);

        modal_form.addControl(input)
                  .addControl(toggle_canBreak)
                  .addControl(toggle_canPlace)
                  .addControl(toggle_canPvp)
                  .addControl(toggle_canTakeDamage);

        modal_form.setOnClose([=](endstone::Player* player)
        {
            player->sendMessage(wg + "This process has been terminated start over");
            processOngoing = false;
            oneOutOfTwo = false;
            requestingPlayer = "";
            newRegionName = "";
        });

        modal_form.setOnSubmit([=](endstone::Player* player, std::string response)
        {
            response = response.substr(1, response.length() - 2);

            std::stringstream ss(response);
            std::string value;
            int index = 0;

            while (std::getline(ss, value, ','))
            {
                value.erase(std::remove(value.begin(), value.end(), ' '), value.end());

                switch (index)
                {
                case 0:
                    newRegionName = value.substr(1, value.length() - 2);
                    break;
                case 1:
                    settup_region_.permissions.canPlace = (value == "true");
                    break;
                case 2:
                    settup_region_.permissions.canBreak = (value == "true");
                    break;
                case 3:
                    settup_region_.permissions.canPvp = (value == "true");
                    break;
                case 4:
                    settup_region_.permissions.canTakeDamage = (value == "true");
                    break;
                default:
                    break;
                }
                ++index;
            }

            writeRegionToFile(configPath, newRegionName, settup_region_);
            player->sendMessage(wg + "Region saved !");
            player->sendMessage(wg + "Restarting the server is required");
        });
        player.sendForm(modal_form);
    }

    bool locatedInsideRegion(const endstone::Location& location, const int& number)
    {
        for (auto it = Regions.begin(); it != Regions.end(); it++)
        {
            int x = location.getX(), y = location.getY(), z = location.getZ();
            int maxX = it->second.boundaries.x1, minX = it->second.boundaries.x2;
            int maxY = it->second.boundaries.y1, minY = it->second.boundaries.y2;
            int maxZ = it->second.boundaries.z1, minZ = it->second.boundaries.z2;
            if (maxX < minX) std::swap(maxX, minX);
            if (maxY < minY) std::swap(maxY, minY);
            if (maxZ < minZ) std::swap(maxZ, minZ);
            if (maxX >= x && x >= minX && maxY >= y && y >= minY && maxZ >= z && z >= minZ)
            {
                switch (number)
                {
                case 0: if (!it->second.permissions.canBreak) return true;
                    break;
                case 1: if (!it->second.permissions.canPlace) return true;
                    break;
                case 2: if (!it->second.permissions.canPvp) return true;
                    break;
                case 3: if (!it->second.permissions.canTakeDamage) return true;
                    break;
                default:
                    break;
                }
            }
        }
        return false;
    }

    void confirmDeleteForm(endstone::Player& player)
    {
        endstone::ActionForm confirm_delete;
        confirm_delete.setTitle(endstone::ColorFormat::Bold + "Delete " + delRegion)
                      .addButton(endstone::ColorFormat::Bold + endstone::ColorFormat::Red + "yes",
                                 "textures/ui/icon_trash", [&](endstone::Player* player)
                                 {
                                     deleteRegionFromFile(configPath, delRegion);
                                     player->sendMessage(wg + "Region deleted");
                                     player->sendMessage(wg + "Restart required");
                                 })
                      .addButton(endstone::ColorFormat::Bold + "no");
        player.sendForm(confirm_delete);
    }

    void deleteRegionForm(endstone::Player& player)
    {
        endstone::ActionForm delete_region_form;
        delete_region_form.setTitle(endstone::ColorFormat::Bold + "Select region");
        for (const auto& entry : Regions)
        {
            delete_region_form.addButton(entry.first, "textures/ui/icon_trash", [=](endstone::Player* player)
            {
                delRegion = entry.first;
                confirmDeleteForm(*player);
            });
        }
        player.sendForm(delete_region_form);
    }
};
