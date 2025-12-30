init_() {

}

// self -> player
PlayEasterEggSong() {
    response = "success";
    switch (level.script) {
        case "zombie_theater":
            level thread maps\zombie_theater_amb::play_music_easter_egg(self);
            break;
        case "zombie_pentagon":
            level thread maps\zombie_pentagon_amb::play_music_easter_egg();
            break;
        case "zombie_cod5_sumpf":
            level thread maps\zombie_cod5_sumpf::play_music_easter_egg();
            break;
        case "zombie_cod5_asylum":
            level thread maps\zombie_cod5_asylum::play_music_easter_egg();
            break;
        case "zombie_cod5_factory":
            level thread maps\zombie_cod5_factory::play_music_easter_egg(self);
            break;
        case "zombie_cosmodrome":
            level thread maps\zombie_cosmodrome_amb::play_music_easter_egg(self);
            break;
        default:
            response = "failed";
            break;
    }
    return response;
}
