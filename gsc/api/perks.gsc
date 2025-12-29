#include maps\_zombiemode_perks;

init_() {
    
}

AddPerks(perks) {
    for (i = 0; i < perks.size; i++) {
        self give_perk(perks[i]);
    }
}

RemovePerks(perks) {
    for (i = 0; i < perks.size; i++) {
        self notify(perks[i] + "_stop");
        self waittill("perk_lost");
    }
    self update_perk_hud();
}

NumPerks() {
    return self.num_perks;
}
