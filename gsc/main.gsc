#include maps\_utility;
#include common_scripts\utility;
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
            switch (method) {
                case "AddPerks":
                    Bo1ztAddPerks(args);
                    break;
                case "RemovePerks":
                    Bo1ztRemovePerks(args);
                    break;
                default:
                    break;
            }
            setdvar("bo1zt_gsc_worker_" + workerId, "");
            wait 0.2; // Wait a bit for dvar update since its async
            self notify("bo1zt::Worker" + workerId + "::" + "success");
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
