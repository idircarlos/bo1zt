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

    // Give tactical grenades first (they don't count towards weapon capacity)
    for (i = 0; i < weapons.size; i++) {
        if (_IsTacticalGranade(weapons[i]) && _IsTacticalEligible(weapons[i])) {
            _GiveTactical(weapons[i]);
            self GiveMaxAmmo(weapons[i]);
        }
    }

    // Give regular weapons, removing old ones if needed
    for (i = 0; i < weapons.size; i++) {
        if (_IsTacticalGranade(weapons[i])) {
            continue; // Skip tacticals, already processed
        }

        if (playerWeapons.size >= weaponsCapacity) {
            oldWeapon = playerWeapons[currentIndex];
            self TakeWeapon(oldWeapon);
            currentIndex = (currentIndex + 1) % weaponsCapacity;
        }

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

_IsTacticalGranade(weapon) {
    if (weapon == "zombie_cymbal_monkey" || 
        weapon == "zombie_black_hole_bomb" ||
        weapon == "zombie_nesting_dolls" ||
        weapon == "zombie_quantum_bomb") {
          return true;  
        }
    return false;
}

_IsTacticalEligible(weapon) {
    if (weapon == "zombie_cymbal_monkey") {
        return true;
    }
    if (weapon == "zombie_black_hole_bomb") {
        return level.script == "zombie_cosmodrome" || level.script == "zombie_moon" || level.script == "zombie_temple";
    }
    if (weapon == "zombie_nesting_dolls") {
        return level.script == "zombie_cosmodrome" || level.script == "zombie_coast";
    }
    if (weapon == "zombie_quantum_bomb") {
        return level.script == "zombie_moon";
    }
    return false;
}

_GiveTactical(weapon) {
    if (weapon == "zombie_cymbal_monkey") {
        self maps\_zombiemode_weap_cymbal_monkey::player_give_cymbal_monkey();
    } else if (weapon == "zombie_black_hole_bomb") {
        self maps\_zombiemode_weap_black_hole_bomb::player_give_black_hole_bomb();
    } else if (weapon == "zombie_nesting_dolls") {
        self maps\_zombiemode_weap_nesting_dolls::player_give_nesting_dolls();
    } else if (weapon == "zombie_quantum_bomb") {
        self maps\_zombiemode_weap_quantum_bomb::player_give_quantum_bomb();
    }
}
