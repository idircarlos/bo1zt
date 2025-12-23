#include maps\_utility;
#include common_scripts\utility;
#include maps\_zombiemode_utility;
#include maps\_hud_util;
#include maps\_zombiemode_perks;

main()
{
    self thread onPlayerConnect();
}

onPlayerConnect()
{
    for(;;)
    {
        level waittill("connected", player);
        player thread bo1ztSetupWorkers();
        player thread onPlayerSpawned();
        player thread onZombieKilled();
        player thread onPowerupDropped();
    }
}

onPlayerSpawned()
{
    self endon("disconnect");
    for(;;)
    {
        self waittill("spawned_player");
        self iprintln("Spawned raygun!");
        self GiveWeapon("ray_gun_zm");
        self SetWeaponAmmoStock("ray_gun_zm", 1000);
        self SwitchToWeapon("ray_gun_zm"); 
    }
}

onZombieKilled() {
    self endon("disconnect");
    self waittill("spawned_player");
    for(;;)
    {
        level waittill("zom_kill", zombie);
        self notify("bo1zt::Level::TotalZombiesKilled", level.total_zombies_killed);
    }
}

onPowerupDropped() {
    self endon("disconnect");
    self waittill("spawned_player");
    for(;;)
    {
        preIndex = level.zombie_powerup_index;
        level waittill("powerup_dropped", powerup);
        self notify("bo1zt::Level::Powerup::Dropped", getPowerupId(powerup.powerup_name));
        postIndex = level.zombie_powerup_index;
        if (postIndex < preIndex) {
            self notify("bo1zt::Level::Powerup::NewCycle");
        }
    }
}

getPowerupId(powerupName) {
    switch (powerupName) {
        case "full_ammo": return 0;
        case "insta_kill": return 1;
        case "nuke": return 2;
        case "double_points": return 3;
        case "carpenter": return 4;
        case "fire_sale": return 5;
        case "minigun": return 6;
        default: return -1;
    }
}


bo1ztSetupWorkers()
{
    self endon("disconnect");
    for (i = 0; i < 10; i++) {
        setdvar("bo1zt_gsc_worker_" + i, "");
        self thread bo1ztSetupWorker(i);         
    }
}

bo1ztSetupWorker(workerId)
{
    self endon("disconnect");
    while (1) {
        request = getdvar("bo1zt_gsc_worker_" + workerId);
        if (request != "") {
            requestTokens = strTok(request, "::");
            method = requestTokens[1];
            args = strTok(requestTokens[2], ",");
            result = "success";
            switch (method) {
                case "AddPerks":
                    Bo1ztAddPerks(args);
                    break;
                case "RemovePerks":
                    Bo1ztRemovePerks(args);
                    break;
                case "StaticBox":
                    result = Bo1ztStaticBox(args);
                    break;
                default:
                    break;
            }
            setdvar("bo1zt_gsc_worker_" + workerId, "");
            wait 0.2; // Wait a bit for dvar update since its async
            self notify("bo1zt::Worker" + workerId + "::" + result);
        }
        wait 0.05;
    }
}

Bo1ztAddPerks(perks)
{
    for (i = 0; i < perks.size; i++) {
        self give_perk(perks[i]);
    }
}

Bo1ztRemovePerks(perks)
{
    for (i = 0; i < perks.size; i++) {
        self notify(perks[i] + "_stop");
        self waittill("perk_lost");
    }
    self update_perk_hud();
}

Bo1ztStaticBox(args)
{        
    if (args.size == 0 || args[0] == "") {
        movable = getdvar("magic_chest_movable");
        if (movable == "1") {
            return "0";
        } else {
            return "1";
        }
    } else {
        if (args[0] == "1") {
            setdvar("magic_chest_movable", "0");
        } else {
            setdvar("magic_chest_movable", "1");
        }
        return "success";
    }
}
