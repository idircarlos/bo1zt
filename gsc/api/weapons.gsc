#include common_scripts\utility;

init_() {
    
}

GiveWeapons(weapons) {
    if (weapons.size == 0) {
        return "fail";
    }
    playerWeapons = self GetWeaponsListPrimaries();
    currentWeapon = self GetCurrentWeapon();
    
    hasMuleKick = self HasPerk("specialty_additionalprimaryweapon");
    weaponsCapacity = 2;
    if (hasMuleKick) {
        weaponsCapacity = 3;
    }

    // Find index of current weapon
    currentIndex = 0;
    for (j = 0; j < playerWeapons.size; j++) {
        if (playerWeapons[j] == currentWeapon) {
            currentIndex = j;
            break;
        }
    }

    // Give weapons, removing old ones if needed
    for (i = 0; i < weapons.size; i++) {
        if (playerWeapons.size >= weaponsCapacity) {
            oldWeapon = playerWeapons[currentIndex];
            self TakeWeapon(oldWeapon);
            currentIndex = (currentIndex + 1) % weaponsCapacity;
        }

        // Give weapon and max ammo
        self GiveWeapon(weapons[i]);
        self GiveMaxAmmo(weapons[i]);

        // Update local weapon list
        playerWeapons = self GetWeaponsListPrimaries();
    }

    // Switch to the first given weapon
    self SwitchToWeapon(playerWeapons[0]);

    return "success";
}

TakeWeapons() {
    weapons = self GetWeaponsListPrimaries();
    for (i = 0; i < weapons.size; i++) {
        self TakeWeapon(weapons[i]);
    }
    return "success";
}
