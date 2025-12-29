#include common_scripts\utility;

init_() {
    self thread onPlayerSpawned();
    self thread onZombieKilled();
    self thread onPowerupDropped();
    self thread onPerkBought();
    self thread onPerkLost();
}

onPlayerSpawned() {
    self endon("disconnect");
    while(1) {
        self waittill("spawned_player");
        self GiveWeapon("ray_gun_zm");
        self SetWeaponAmmoStock("ray_gun_zm", 1000);
        self SwitchToWeapon("ray_gun_zm");
    }
}

onZombieKilled() {
    self endon("disconnect");
    self waittill("spawned_player");
    while(1) {
        level waittill("zom_kill", zombie);
        self notify("bo1zt::Level::TotalZombiesKilled", level.total_zombies_killed);
    }
}

onPowerupDropped() {
    self endon("disconnect");
    self waittill("spawned_player");
    while(1) {
        preIndex = level.zombie_powerup_index;
        level waittill("powerup_dropped", powerup);
        postIndex = level.zombie_powerup_index;
        if (flag("dog_round")) {
            continue;
        }
        if (postIndex < preIndex) {
            self notify("bo1zt::Level::Powerup::NewCycle");
        }
        self notify("bo1zt::Level::Powerup::Dropped", getPowerupId(powerup.powerup_name));   
        if (powerup.powerup_name == "free_perk") {
            powerup thread onFreePerkPowerupGrabbed(self);
        }
    }
}

onFreePerkPowerupGrabbed(player) {
    self waittill("powerup_grabbed");
    self notify("bo1zt::Player::NumPerks", player.num_perks);
}

onPerkBought() {
    self endon("disconnect");
    self waittill("spawned_player");
    while(1) {
        self waittill("perk_bought");
        self notify("bo1zt::Player::NumPerks", self.num_perks);
    }
}

onPerkLost() {
    self endon("disconnect");
    self waittill("spawned_player");
    while(1) {
        self waittill("perk_lost");
        self notify("bo1zt::Player::NumPerks", self.num_perks);
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
        case "free_perk": return 7;
        default: return -1;
    }
}
