#include maps\_utility;
#include common_scripts\utility;
#include maps\_hud_util;

main()
{
    self thread onPlayerConnect();
}

onPlayerConnect()
{
    for(;;)
    {
        level waittill("connected", player);
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
