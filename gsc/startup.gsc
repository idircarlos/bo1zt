init_() {
    self thread overrideCharacter();
}

overrideCharacter() {
    multiplayer = GetNumExpectedPlayers() > 1;
    if (multiplayer || level.script == "zombie_pentagon") return; // Not supporting this on MP and Five
    self.characterId = int(getdvar("bo1zt_character"));
    if (self.characterId == 4) return; // characterId == 4 bo1zt means Random, so we skip the override.
    level.zombiemode_give_player_model_override = ::givePlayerModelOverride;
}

givePlayerModelOverride(entity_num) {
    self.zm_random_char = self.characterId;
    self.entity_num = self.characterId;
    switch( self.characterId ) {
        case 0: 
            giveDempseyModel();
            break;
        case 1:
            giveNikolaiModel();
            break;
        case 2:
            giveTakeoModel();
            break;
        case 3:
            giveRichtofenModel();
            break;
    }
}

giveDempseyModel() {
    switch (level.script) {
        case "zombie_theater":
        case "zombie_temple":
        case "zombie_cod5_sumpf":
        case "zombie_cod5_asylum":
        case "zombie_cod5_factory":
        case "zombie_cod5_prototype":
            character\c_usa_dempsey_zt::main();
            break;
        case "zombie_cosmodrome":
            character\c_usa_dempsey_dlc2::main();
            break;
        case "zombie_moon":
            character\c_usa_dempsey_dlc5::main();
            level._num_overriden_models++; // Only needed on moon
            break;
        case "zombie_coast":
            character\c_zom_sarah_michelle_gellar_player::main();
        case "zombie_pentagon": iprintln("Unreacheable!");
    }
}

giveNikolaiModel() {
    switch (level.script) {
        case "zombie_theater":
        case "zombie_temple":
        case "zombie_cod5_sumpf":
        case "zombie_cod5_asylum":
        case "zombie_cod5_factory":
        case "zombie_cod5_prototype":
            character\c_rus_nikolai_zt::main();
            break;
        case "zombie_cosmodrome":
            character\c_rus_nikolai_dlc2::main();
            break;
        case "zombie_moon":
            character\c_rus_nikolai_dlc5::main();
            level._num_overriden_models++; // Only needed on moon
            break;
        case "zombie_coast":
            character\c_zom_robert_englund_player::main();
        case "zombie_pentagon": iprintln("Unreacheable!");
    }
}

giveTakeoModel() {
    switch (level.script) {
        case "zombie_theater":
        case "zombie_temple":
        case "zombie_cod5_sumpf":
        case "zombie_cod5_asylum":
        case "zombie_cod5_factory":
        case "zombie_cod5_prototype":
            character\c_jap_takeo_zt::main();
            break;
        case "zombie_cosmodrome":
            character\c_jap_takeo_dlc2::main();
            break;
        case "zombie_moon":
            character\c_jap_takeo_dlc5::main();
            level._num_overriden_models++; // Only needed on moon
            break;
        case "zombie_coast":
            character\c_zom_danny_trejo_player::main();
        case "zombie_pentagon": iprintln("Unreacheable!");
    }
}

giveRichtofenModel() {
    switch (level.script) {
        case "zombie_theater":
        case "zombie_temple":
        case "zombie_cod5_sumpf":
        case "zombie_cod5_asylum":
        case "zombie_cod5_factory":
        case "zombie_cod5_prototype":
            character\c_ger_richtofen_zt::main();
            break;
        case "zombie_cosmodrome":
            character\c_ger_richtofen_dlc2::main();
            break;
        case "zombie_moon":
            character\c_ger_richtofen_dlc5::main();
            level._num_overriden_models++; // Only needed on moon
            break;
        case "zombie_coast":
            character\c_zom_michael_rooker_player::main();
        case "zombie_pentagon": iprintln("Unreacheable!");
    }
}
