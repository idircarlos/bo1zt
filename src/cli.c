#include "cli.h"
#include "client.h"
#include "client/cheats.h"
#include "client/commands.h"
#include "client/game.h"
#include "client/state.h"
#include "client/round.h"
#include "client/player.h"
#include "client/stats.h"
#include "client/trade.h"
#include "client/graphics.h"
#include "client/customizer.h"
#include "client/widgets.h"
#include "client/binds.h"
#include "client/actions.h"
#include "service.h"

#include "argparse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int reportClientError(const Client *client, ClientResult result) {
    if (result == CLIENT_ERR_UNREACHABLE) {
        fprintf(stderr, "error: cannot reach bo1zt on 127.0.0.1 (is it running?)\n");
        return 1;
    }
    const char *message = clientLastErrorMessage(client);
    fprintf(stderr, "error: %s\n", (message && *message) ? message : "request failed");
    return 1;
}

static int applyGameConfigAssignment(Client *client, const char *key, const char *value) {
    ClientResult r;
    if (strcmp(key, "location") == 0)       r = clientSetGameLocation(client, value);
    else if (strcmp(key, "hostname") == 0)  r = clientSetGameHostname(client, value);
    else if (strcmp(key, "character") == 0) r = clientSetGameCharacter(client, value);
    else {
        fprintf(stderr, "error: unknown game config field '%s' (location|hostname|character)\n", key);
        return 2;
    }
    return (r == CLIENT_OK) ? 0 : reportClientError(client, r);
}

static int cmdGameConfig(Client *client, const Namespace *ns) {
    size_t count = GetCount(ns, "set");
    if (count == 0) {
        GameConfigInfo cfg;
        ClientResult r = clientGetGameConfig(client, &cfg);
        if (r != CLIENT_OK) return reportClientError(client, r);
        printf("location: %s\n", cfg.location[0] ? cfg.location : "-");
        printf("hostname: %s\n", cfg.hostname[0] ? cfg.hostname : "-");
        printf("character: %s\n", cfg.character[0] ? cfg.character : "-");
        return 0;
    }

    for (size_t i = 0; i < count; i++) {
        const char *token = GetStringAt(ns, "set", i);
        const char *eq = token ? strchr(token, '=') : NULL;
        if (!eq || eq == token || eq[1] == '\0') {
            fprintf(stderr, "error: expected KEY=value, got '%s'\n", token ? token : "");
            return 2;
        }
        char key[64];
        size_t n = (size_t)(eq - token);
        if (n >= sizeof(key)) n = sizeof(key) - 1;
        memcpy(key, token, n);
        key[n] = '\0';
        int rc = applyGameConfigAssignment(client, key, eq + 1);
        if (rc != 0) return rc;
    }
    printf("set %zu field(s)\n", count);
    return 0;
}

static int cmdGame(Client *client, const Namespace *ns) {
    const char *action = GetString(ns, "action");
    if (!action || strcmp(action, "status") == 0) {
        GameStatus status;
        ClientResult r = clientGetGameStatus(client, &status);
        if (r != CLIENT_OK) return reportClientError(client, r);
        printf("attached: %s\n", status.attached ? "yes" : "no");
        printf("running: %s\n", status.running ? "yes" : "no");
        printf("ready: %s\n", status.ready ? "yes" : "no");
        printf("window-focused: %s\n", status.windowFocused ? "yes" : "no");
        return 0;
    }
    if (strcmp(action, "config") == 0) return cmdGameConfig(client, ns);

    ClientResult r;
    if (strcmp(action, "launch") == 0)       r = clientLaunchGame(client);
    else if (strcmp(action, "close") == 0)   r = clientCloseGame(client);
    else if (strcmp(action, "restart") == 0) r = clientRestartGame(client);
    else {
        fprintf(stderr, "error: unknown game action '%s'\n", action);
        return 2;
    }
    if (r != CLIENT_OK) return reportClientError(client, r);
    printf("ok\n");
    return 0;
}

static int cmdState(Client *client) {
    GameState s;
    ClientResult r = clientGetState(client, &s);
    if (r != CLIENT_OK) return reportClientError(client, r);
    const char *zombies = s.isZombiesGameOngoing
        ? (s.isZombiesGamePaused ? "paused" : "ongoing")
        : "none";
    printf("attached: %s\n", s.isGameAttached ? "yes" : "no");
    printf("zombies-game: %s\n", zombies);
    printf("game-resets: %d\n", s.gameResets);
    printf("level: %s\n", s.level[0] ? s.level : "-");
    printf("elapsed: %d\n", s.elapsed);
    printf("movement-speed: %g\n", s.movementSpeed);
    printf("round: %d\n", s.round);
    printf("entities: %d/%d\n", s.entitiesCurrent, s.entitiesMax);
    return 0;
}

static int cmdCheat(Client *client, const Namespace *ns) {
    const char *name = GetString(ns, "name");
    CheatName cheat;
    if (!clientCheatFromName(name, &cheat)) {
        fprintf(stderr, "error: unknown cheat '%s'\n", name ? name : "");
        return 2;
    }

    if (WasPresent(ns, "value")) {
        bool enabled = (strcmp(GetString(ns, "value"), "on") == 0);
        ClientResult r = clientSetCheat(client, cheat, enabled);
        if (r != CLIENT_OK) return reportClientError(client, r);
        printf("%s: %s\n", name, enabled ? "on" : "off");
        return 0;
    }

    bool enabled = false;
    ClientResult r = clientGetCheat(client, cheat, &enabled);
    if (r != CLIENT_OK) return reportClientError(client, r);
    printf("%s: %s\n", name, enabled ? "on" : "off");
    return 0;
}

static int cmdCheats(Client *client, const Namespace *ns) {
    size_t count = GetCount(ns, "set");
    if (count == 0) {
        bool enabled[64];
        int total = 0;
        ClientResult r = clientGetCheats(client, enabled, 64, &total);
        if (r != CLIENT_OK) return reportClientError(client, r);
        for (int i = 0; i < total; i++) {
            printf("%s: %s\n", clientCheatNameAt(i), enabled[i] ? "on" : "off");
        }
        return 0;
    }

    CheatName names[64];
    bool values[64];
    if (count > 64) count = 64;
    for (size_t i = 0; i < count; i++) {
        const char *token = GetStringAt(ns, "set", i);
        const char *eq = token ? strchr(token, '=') : NULL;
        if (!eq || eq == token || eq[1] == '\0') {
            fprintf(stderr, "error: expected NAME=on|off, got '%s'\n", token ? token : "");
            return 2;
        }
        const char *value = eq + 1;
        if (strcmp(value, "on") == 0) values[i] = true;
        else if (strcmp(value, "off") == 0) values[i] = false;
        else {
            fprintf(stderr, "error: value must be 'on' or 'off', got '%s'\n", value);
            return 2;
        }
        char name[64];
        size_t n = (size_t)(eq - token);
        if (n >= sizeof(name)) n = sizeof(name) - 1;
        memcpy(name, token, n);
        name[n] = '\0';
        if (!clientCheatFromName(name, &names[i])) {
            fprintf(stderr, "error: unknown cheat '%s'\n", name);
            return 2;
        }
    }

    ClientResult r = clientSetCheats(client, names, values, (int)count);
    if (r != CLIENT_OK) return reportClientError(client, r);
    printf("set %zu cheat(s)\n", count);
    return 0;
}

static int cmdCommands(Client *client) {
    CommandInfo commands[64];
    int count = 0;
    ClientResult r = clientGetCommands(client, commands, 64, &count);
    if (r != CLIENT_OK) return reportClientError(client, r);
    for (int i = 0; i < count; i++) {
        printf("%-24s %s\n", commands[i].usage, commands[i].description);
    }
    return 0;
}

static int cmdRound(Client *client, const Namespace *ns) {
    if (WasPresent(ns, "number")) {
        long n = GetInt(ns, "number");
        ClientResult r = clientSetRound(client, (int)n);
        if (r != CLIENT_OK) return reportClientError(client, r);
        printf("round set to %ld\n", n);
        return 0;
    }
    int round = 0;
    ClientResult r = clientGetRound(client, &round);
    if (r != CLIENT_OK) return reportClientError(client, r);
    printf("round: %d\n", round);
    return 0;
}

static int cmdTeleport(Client *client, const Namespace *ns) {
    double x = GetFloat(ns, "x");
    double y = GetFloat(ns, "y");
    double z = GetFloat(ns, "z");
    ClientResult r = clientTeleport(client, (float)x, (float)y, (float)z);
    if (r != CLIENT_OK) return reportClientError(client, r);
    printf("teleported to %g %g %g\n", x, y, z);
    return 0;
}

static int cmdPosition(Client *client) {
    TeleportCoords coords;
    ClientResult r = clientGetPosition(client, &coords);
    if (r != CLIENT_OK) return reportClientError(client, r);
    printf("x: %g  y: %g  z: %g\n", coords.x, coords.y, coords.z);
    return 0;
}

static int cmdHealth(Client *client) {
    char version[64];
    ClientResult r = clientGetVersion(client, version, sizeof(version));
    if (r != CLIENT_OK) return reportClientError(client, r);
    printf("bo1zt %s\n", version);
    return 0;
}

static void printColorLine(const char *name, Color c) {
    printf("%s: %d,%d,%d,%d\n", name, c.r, c.g, c.b, c.a);
}

static void printWidget(const char *name, const WidgetConfig *w) {
    printf("%s: %s font=\"%s\" size=%d color=%d,%d,%d,%d hide-outside-game=%s rect=%u,%u,%u,%u\n",
           name, w->enabled ? "on" : "off", w->font, w->fontSize,
           w->textColor.r, w->textColor.g, w->textColor.b, w->textColor.a,
           w->hideOutsideGame ? "yes" : "no",
           w->rect.x, w->rect.y, w->rect.w, w->rect.h);
}

static bool parseIntList(const char *value, long *out, int count, long min, long max) {
    const char *p = value;
    for (int i = 0; i < count; i++) {
        char *end = NULL;
        long v = strtol(p, &end, 10);
        if (end == p) return false;
        if (v < min || v > max) return false;
        out[i] = v;
        p = end;
        if (i < count - 1) {
            if (*p != ',') return false;
            p++;
        }
    }
    return *p == '\0';
}

static int applyWidgetAssignment(WidgetConfig *w, const char *key, const char *value) {
    if (strcmp(key, "enabled") == 0 || strcmp(key, "hide-outside-game") == 0) {
        if (strcmp(value, "on") != 0 && strcmp(value, "off") != 0) {
            fprintf(stderr, "error: %s expects on|off\n", key);
            return 2;
        }
        bool on = (strcmp(value, "on") == 0);
        if (strcmp(key, "enabled") == 0) w->enabled = on;
        else w->hideOutsideGame = on;
        return 0;
    }
    if (strcmp(key, "font") == 0) {
        snprintf(w->font, sizeof(w->font), "%s", value);
        return 0;
    }
    if (strcmp(key, "font-size") == 0) {
        long v[1];
        if (!parseIntList(value, v, 1, 1, 1000)) {
            fprintf(stderr, "error: font-size expects a positive number\n");
            return 2;
        }
        w->fontSize = (int)v[0];
        return 0;
    }
    if (strcmp(key, "color") == 0) {
        long c[4];
        if (!parseIntList(value, c, 4, 0, 255)) {
            fprintf(stderr, "error: color expects R,G,B,A each in 0..255\n");
            return 2;
        }
        w->textColor.r = (uint8_t)c[0];
        w->textColor.g = (uint8_t)c[1];
        w->textColor.b = (uint8_t)c[2];
        w->textColor.a = (uint8_t)c[3];
        return 0;
    }
    if (strcmp(key, "rect") == 0) {
        long v[4];
        if (!parseIntList(value, v, 4, 0, 0x7fffffff)) {
            fprintf(stderr, "error: rect expects X,Y,W,H\n");
            return 2;
        }
        w->rect.x = (uint32_t)v[0];
        w->rect.y = (uint32_t)v[1];
        w->rect.w = (uint32_t)v[2];
        w->rect.h = (uint32_t)v[3];
        return 0;
    }
    fprintf(stderr, "error: unknown widget key '%s' "
                    "(enabled|font|font-size|color|hide-outside-game|rect)\n", key);
    return 2;
}

static int cmdWidgets(Client *client, const Namespace *ns) {
    if (!WasPresent(ns, "name")) {
        int count = clientWidgetCount();
        for (int i = 0; i < count; i++) {
            const char *name = clientWidgetNameAt(i);
            WidgetConfig w;
            ClientResult r = clientGetWidget(client, name, &w);
            if (r != CLIENT_OK) return reportClientError(client, r);
            printWidget(name, &w);
        }
        return 0;
    }

    const char *name = GetString(ns, "name");
    WidgetConfig w;
    ClientResult r = clientGetWidget(client, name, &w);
    if (r != CLIENT_OK) return reportClientError(client, r);

    size_t count = GetCount(ns, "set");
    if (count == 0) {
        printWidget(name, &w);
        return 0;
    }

    for (size_t i = 0; i < count; i++) {
        const char *token = GetStringAt(ns, "set", i);
        const char *eq = token ? strchr(token, '=') : NULL;
        if (!eq || eq == token || eq[1] == '\0') {
            fprintf(stderr, "error: expected KEY=value, got '%s'\n", token ? token : "");
            return 2;
        }
        char key[64];
        size_t n = (size_t)(eq - token);
        if (n >= sizeof(key)) n = sizeof(key) - 1;
        memcpy(key, token, n);
        key[n] = '\0';
        int rc = applyWidgetAssignment(&w, key, eq + 1);
        if (rc != 0) return rc;
    }

    r = clientSetWidget(client, name, &w);
    if (r != CLIENT_OK) return reportClientError(client, r);
    printWidget(name, &w);
    return 0;
}

static int cmdCustomizer(Client *client) {
    CustomizerConfig c;
    ClientResult r = clientGetCustomizer(client, &c);
    if (r != CLIENT_OK) return reportClientError(client, r);
    printColorLine("score-background", c.scoreBackground);
    printColorLine("score-player-1", c.scorePlayer1);
    printColorLine("score-player-2", c.scorePlayer2);
    printColorLine("score-player-3", c.scorePlayer3);
    printColorLine("score-player-4", c.scorePlayer4);
    printColorLine("reload-warn-primary", c.reloadWarnPrimary);
    printColorLine("reload-warn-secondary", c.reloadWarnSecondary);
    printColorLine("low-ammo-warn-primary", c.lowAmmoWarnPrimary);
    printColorLine("low-ammo-warn-secondary", c.lowAmmoWarnSecondary);
    printColorLine("no-ammo-warn-primary", c.noAmmoWarnPrimary);
    printColorLine("no-ammo-warn-secondary", c.noAmmoWarnSecondary);
    printf("scoreboard-transparency: %d\n", c.scoreboardTransparency);
    printf("points-transparency: %d\n", c.pointsTransparency);
    printf("warning-frequency: %d\n", c.warningTransitionsFrequency);
    printf("warning-min: %d\n", c.warningTransitionsMin);
    printf("warning-max: %d\n", c.warningTransitionsMax);
    return 0;
}

static void printBinds(const BindsConfig *b) {
    if (b->bindCount == 0) {
        printf("(no binds)\n");
        return;
    }
    for (int i = 0; i < b->bindCount; i++) {
        printf("%s -> %s\n", b->binds[i].keyName, b->binds[i].command);
    }
}

static int bindsIndexOf(const BindsConfig *b, const char *keyName) {
    for (int i = 0; i < b->bindCount; i++) {
        if (_stricmp(b->binds[i].keyName, keyName) == 0) return i;
    }
    return -1;
}

static int cmdBindsAdd(Client *client, const Namespace *ns) {
    size_t count = GetCount(ns, "args");
    if (count == 0) {
        fprintf(stderr, "error: expected at least one KEY=command\n");
        return 2;
    }

    BindsConfig b;
    ClientResult r = clientGetBinds(client, &b);
    if (r != CLIENT_OK) return reportClientError(client, r);

    for (size_t i = 0; i < count; i++) {
        const char *token = GetStringAt(ns, "args", i);
        const char *eq = token ? strchr(token, '=') : NULL;
        if (!eq || eq == token || eq[1] == '\0') {
            fprintf(stderr, "error: expected KEY=command, got '%s'\n", token ? token : "");
            return 2;
        }
        char key[MAX_KEY_NAME_LENGTH];
        size_t n = (size_t)(eq - token);
        if (n >= sizeof(key)) {
            fprintf(stderr, "error: key name too long: '%s'\n", token);
            return 2;
        }
        memcpy(key, token, n);
        key[n] = '\0';
        const char *command = eq + 1;

        int idx = bindsIndexOf(&b, key);
        if (idx < 0) {
            if (b.bindCount >= MAX_BINDS) {
                fprintf(stderr, "error: bind limit reached (%d)\n", MAX_BINDS);
                return 2;
            }
            idx = b.bindCount++;
        }
        snprintf(b.binds[idx].keyName, sizeof(b.binds[idx].keyName), "%s", key);
        snprintf(b.binds[idx].command, sizeof(b.binds[idx].command), "%s", command);
    }

    r = clientSetBinds(client, &b);
    if (r != CLIENT_OK) return reportClientError(client, r);
    printBinds(&b);
    return 0;
}

static int cmdBindsRemove(Client *client, const Namespace *ns) {
    size_t count = GetCount(ns, "args");
    if (count == 0) {
        fprintf(stderr, "error: expected at least one key name\n");
        return 2;
    }

    BindsConfig b;
    ClientResult r = clientGetBinds(client, &b);
    if (r != CLIENT_OK) return reportClientError(client, r);

    int removed = 0;
    for (size_t i = 0; i < count; i++) {
        const char *key = GetStringAt(ns, "args", i);
        int idx = bindsIndexOf(&b, key);
        if (idx < 0) {
            fprintf(stderr, "error: no bind for key '%s'\n", key ? key : "");
            return 2;
        }
        for (int j = idx; j < b.bindCount - 1; j++) b.binds[j] = b.binds[j + 1];
        b.bindCount--;
        removed++;
    }

    r = clientSetBinds(client, &b);
    if (r != CLIENT_OK) return reportClientError(client, r);
    printf("removed %d bind(s)\n", removed);
    return 0;
}

static int cmdBinds(Client *client, const Namespace *ns) {
    const char *action = GetString(ns, "action");
    if (!action) {
        BindsConfig b;
        ClientResult r = clientGetBinds(client, &b);
        if (r != CLIENT_OK) return reportClientError(client, r);
        printBinds(&b);
        return 0;
    }
    if (strcmp(action, "add") == 0)   return cmdBindsAdd(client, ns);
    if (strcmp(action, "rm") == 0)    return cmdBindsRemove(client, ns);
    if (strcmp(action, "reset") == 0) {
        ClientResult r = clientResetBinds(client);
        if (r != CLIENT_OK) return reportClientError(client, r);
        printf("binds reset\n");
        return 0;
    }
    fprintf(stderr, "error: unknown binds action '%s' (add|rm|reset)\n", action);
    return 2;
}

static bool isGraphicsBoolKey(const char *key) {
    return strcmp(key, "borderless") == 0 || strcmp(key, "unlimit-fps") == 0 ||
           strcmp(key, "disable-hud") == 0 || strcmp(key, "disable-fog") == 0 ||
           strcmp(key, "fullbright") == 0 || strcmp(key, "colorized") == 0;
}

static void printGraphics(const GraphicsConfig *g) {
    printf("fov: %d\n", g->fov);
    printf("fov-scale: %d\n", g->fovScale);
    printf("fps-cap: %d\n", g->fpsCap);
    printf("borderless: %s\n", g->borderless ? "on" : "off");
    printf("unlimit-fps: %s\n", g->unlimitFps ? "on" : "off");
    printf("disable-hud: %s\n", g->disableHud ? "on" : "off");
    printf("disable-fog: %s\n", g->disableFog ? "on" : "off");
    printf("fullbright: %s\n", g->fullbright ? "on" : "off");
    printf("colorized: %s\n", g->colorized ? "on" : "off");
}

static int applyGraphicsAssignment(GraphicsConfig *g, const char *key, const char *value) {
    bool on = (strcmp(value, "on") == 0);
    if (isGraphicsBoolKey(key)) {
        if (strcmp(value, "on") != 0 && strcmp(value, "off") != 0) {
            fprintf(stderr, "error: %s expects on|off\n", key);
            return 2;
        }
        if (strcmp(key, "borderless") == 0) g->borderless = on;
        else if (strcmp(key, "unlimit-fps") == 0) g->unlimitFps = on;
        else if (strcmp(key, "disable-hud") == 0) g->disableHud = on;
        else if (strcmp(key, "disable-fog") == 0) g->disableFog = on;
        else if (strcmp(key, "fullbright") == 0) g->fullbright = on;
        else if (strcmp(key, "colorized") == 0) g->colorized = on;
        return 0;
    }
    char *end = NULL;
    long v = strtol(value, &end, 10);
    if (end == value || *end != '\0') {
        fprintf(stderr, "error: %s expects a number\n", key);
        return 2;
    }
    if (strcmp(key, "fov") == 0) g->fov = (int)v;
    else if (strcmp(key, "fov-scale") == 0) g->fovScale = (int)v;
    else if (strcmp(key, "fps-cap") == 0) g->fpsCap = (int)v;
    else {
        fprintf(stderr, "error: unknown graphics key '%s'\n", key);
        return 2;
    }
    return 0;
}

static int cmdGraphics(Client *client, const Namespace *ns) {
    GraphicsConfig gfx;
    ClientResult r = clientGetGraphics(client, &gfx);
    if (r != CLIENT_OK) return reportClientError(client, r);

    size_t count = GetCount(ns, "set");
    if (count == 0) {
        printGraphics(&gfx);
        return 0;
    }

    for (size_t i = 0; i < count; i++) {
        const char *token = GetStringAt(ns, "set", i);
        const char *eq = token ? strchr(token, '=') : NULL;
        if (!eq || eq == token || eq[1] == '\0') {
            fprintf(stderr, "error: expected KEY=value, got '%s'\n", token ? token : "");
            return 2;
        }
        char key[64];
        size_t n = (size_t)(eq - token);
        if (n >= sizeof(key)) n = sizeof(key) - 1;
        memcpy(key, token, n);
        key[n] = '\0';
        int rc = applyGraphicsAssignment(&gfx, key, eq + 1);
        if (rc != 0) return rc;
    }

    r = clientSetGraphics(client, &gfx);
    if (r != CLIENT_OK) return reportClientError(client, r);
    printGraphics(&gfx);
    return 0;
}

static int cmdPerks(Client *client, const Namespace *ns) {
    if (!WasPresent(ns, "action")) {
        int count = 0;
        ClientResult r = clientGetPerkCount(client, &count);
        if (r != CLIENT_OK) return reportClientError(client, r);
        printf("perks: %d\n", count);
        return 0;
    }
    const char *action = GetString(ns, "action");
    size_t count = GetCount(ns, "perks");
    if (count == 0) {
        fprintf(stderr, "error: specify at least one perk\n");
        return 2;
    }

    Perk perks[16];
    if (count > 16) count = 16;
    for (size_t i = 0; i < count; i++) {
        const char *name = GetStringAt(ns, "perks", i);
        if (!clientPerkFromName(name, &perks[i])) {
            fprintf(stderr, "error: unknown perk '%s'\n", name ? name : "");
            return 2;
        }
    }

    ClientResult r = (strcmp(action, "add") == 0)
        ? clientAddPerks(client, perks, (int)count)
        : clientRemovePerks(client, perks, (int)count);
    if (r != CLIENT_OK) return reportClientError(client, r);
    printf("%s %zu perk(s)\n", action, count);
    return 0;
}

static int cmdWeapons(Client *client, const Namespace *ns) {
    size_t count = GetCount(ns, "weapons");
    if (count == 0) {
        fprintf(stderr, "error: specify at least one weapon\n");
        return 2;
    }

    if (count == 1 && strcmp(GetStringAt(ns, "weapons", 0), "take") == 0) {
        ClientResult r = clientTakeWeapons(client);
        if (r != CLIENT_OK) return reportClientError(client, r);
        printf("took all weapons\n");
        return 0;
    }

    Weapon weapons[32];
    if (count > 32) count = 32;
    for (size_t i = 0; i < count; i++) {
        const char *name = GetStringAt(ns, "weapons", i);
        if (!clientWeaponFromName(name, &weapons[i])) {
            fprintf(stderr, "error: unknown weapon '%s'\n", name ? name : "");
            return 2;
        }
    }

    ClientResult r = clientGiveWeapons(client, weapons, (int)count);
    if (r != CLIENT_OK) return reportClientError(client, r);
    printf("gave %zu weapon(s)\n", count);
    return 0;
}

static void printPlayer(const PlayerAttributes *a) {
    printf("name: %s\n", a->name[0] ? a->name : "-");
    printf("health: %d\n", a->health);
    printf("points: %d\n", a->points);
    printf("kills: %d\n", a->kills);
    printf("headshots: %d\n", a->headshots);
    printf("movement-speed: %g\n", a->movementSpeed);
}

static int applyPlayerAssignment(Client *client, const char *key, const char *value) {
    if (strcmp(key, "name") == 0) {
        ClientResult r = clientSetPlayerName(client, value);
        return (r == CLIENT_OK) ? 0 : reportClientError(client, r);
    }
    char *end = NULL;
    long v = strtol(value, &end, 10);
    if (end == value || *end != '\0') {
        fprintf(stderr, "error: %s expects a number\n", key);
        return 2;
    }
    ClientResult r;
    if (strcmp(key, "health") == 0)              r = clientSetPlayerHealth(client, (int)v);
    else if (strcmp(key, "points") == 0)         r = clientSetPlayerPoints(client, (int)v);
    else if (strcmp(key, "kills") == 0)          r = clientSetPlayerKills(client, (int)v);
    else if (strcmp(key, "headshots") == 0)      r = clientSetPlayerHeadshots(client, (int)v);
    else if (strcmp(key, "movement-speed") == 0) r = clientSetPlayerMovementSpeed(client, (int)v);
    else {
        fprintf(stderr, "error: unknown player key '%s'\n", key);
        return 2;
    }
    return (r == CLIENT_OK) ? 0 : reportClientError(client, r);
}

static int cmdPlayer(Client *client, const Namespace *ns) {
    size_t count = GetCount(ns, "set");
    if (count == 0) {
        PlayerAttributes a;
        ClientResult r = clientGetPlayer(client, &a);
        if (r != CLIENT_OK) return reportClientError(client, r);
        printPlayer(&a);
        return 0;
    }

    for (size_t i = 0; i < count; i++) {
        const char *token = GetStringAt(ns, "set", i);
        const char *eq = token ? strchr(token, '=') : NULL;
        if (!eq || eq == token || eq[1] == '\0') {
            fprintf(stderr, "error: expected KEY=value, got '%s'\n", token ? token : "");
            return 2;
        }
        char key[64];
        size_t n = (size_t)(eq - token);
        if (n >= sizeof(key)) n = sizeof(key) - 1;
        memcpy(key, token, n);
        key[n] = '\0';
        int rc = applyPlayerAssignment(client, key, eq + 1);
        if (rc != 0) return rc;
    }
    printf("set %zu attribute(s)\n", count);
    return 0;
}

static void printSpecial(const char *name, int value) {
    if (value < 0) printf("%s: -\n", name);
    else printf("%s: %d\n", name, value);
}

static int cmdStats(Client *client, const Namespace *ns) {
    int round = WasPresent(ns, "round") ? (int)GetInt(ns, "round") : 0;
    Stats s;
    ClientResult r = clientGetStats(client, round, &s);
    if (r != CLIENT_OK) return reportClientError(client, r);
    printf("entities: %d/%d\n", s.entitiesCurrent, s.entitiesMax);
    printf("claymores: %d\n", s.claymores);
    printf("revives: %d\n", s.revives);
    printSpecial("dogs", s.specialDogs);
    printSpecial("monkeys", s.specialMonkeys);
    printSpecial("thief", s.specialThief);
    printf("sph: %g\n", s.sph);
    return 0;
}

static int cmdTrade(Client *client, const Namespace *ns) {
    const char *action = GetString(ns, "action");
    if (!action || strcmp(action, "status") == 0) {
        TradeStatus t;
        ClientResult r = clientGetTrade(client, &t);
        if (r != CLIENT_OK) return reportClientError(client, r);
        printf("running: %s\n", t.running ? "yes" : "no");
        printf("elapsed-ms: %d\n", t.elapsedMs);
        printf("hits: %d\n", t.hits);
        return 0;
    }
    if (strcmp(action, "total") == 0) {
        TradeTotal t;
        ClientResult r = clientGetTradeTotal(client, &t);
        if (r != CLIENT_OK) return reportClientError(client, r);
        printf("trades: %d\n", t.trades);
        printf("total-ms: %d\n", t.totalMs);
        printf("total-hits: %d\n", t.totalHits);
        return 0;
    }
    if (strcmp(action, "start") == 0) {
        ClientResult r = clientStartTrade(client);
        if (r != CLIENT_OK) return reportClientError(client, r);
        printf("trade started\n");
        return 0;
    }
    if (strcmp(action, "end") == 0) {
        TradeStatus t;
        ClientResult r = clientEndTrade(client, &t);
        if (r != CLIENT_OK) return reportClientError(client, r);
        printf("elapsed-ms: %d\nhits: %d\n", t.elapsedMs, t.hits);
        return 0;
    }
    if (strcmp(action, "cancel") == 0) {
        ClientResult r = clientCancelTrade(client);
        if (r != CLIENT_OK) return reportClientError(client, r);
        printf("trade cancelled\n");
        return 0;
    }
    fprintf(stderr, "error: unknown trade action '%s'\n", action);
    return 2;
}

static void buildParser(Parser *p) {
    SetDescription(p, "bo1zt CLI - control a running bo1zt instance over HTTP.");

    Subparsers *sp = AddSubparsers(p, "command");
    SetSubparsersRequired(sp, true);
    SetSubparsersTitle(sp, "commands");

    AddParser(sp, "health", "Server liveness (version)");
    AddParser(sp, "state", "Full state snapshot");
    AddParser(sp, "commands", "Chat-command catalog");

    Parser *game = AddParser(sp, "game", "Game status / lifecycle / config");
    Argument *gameAction = AddArgument(game, "action", NULL);
    ArgNargs(gameAction, NARGS_OPTIONAL);
    ArgChoices(gameAction, "status", "launch", "close", "restart", "config", NULL);
    ArgHelp(gameAction, "status (default), launch, close, restart, or config");
    ArgHelp(ArgNargs(AddArgument(game, "set", NULL), NARGS_ZERO_OR_MORE),
            "for 'config': KEY=value, e.g. character=nikolai hostname=\"My Server\"");

    Parser *cheats = AddParser(sp, "cheats", "List all cheats, or batch-set NAME=on|off");
    ArgHelp(ArgNargs(AddArgument(cheats, "set", NULL), NARGS_ZERO_OR_MORE),
            "cheat assignments, e.g. god=on noclip=off");

    Parser *cheat = AddParser(sp, "cheat", "Get or set a single cheat");
    ArgHelp(AddArgument(cheat, "name", NULL), "cheat name (e.g. god, noclip)");
    ArgChoices(ArgNargs(AddArgument(cheat, "value", NULL), NARGS_OPTIONAL),
               "on", "off", NULL);

    Parser *round = AddParser(sp, "round", "Get current round, or jump to a round");
    ArgType(ArgNargs(AddArgument(round, "number", NULL), NARGS_OPTIONAL), TYPE_INT);

    AddParser(sp, "position", "Player position");

    Parser *teleport = AddParser(sp, "teleport", "Teleport player to coordinates");
    ArgType(AddArgument(teleport, "x", NULL), TYPE_FLOAT);
    ArgType(AddArgument(teleport, "y", NULL), TYPE_FLOAT);
    ArgType(AddArgument(teleport, "z", NULL), TYPE_FLOAT);

    AddParser(sp, "ammo", "Give the player ammo");

    Parser *perks = AddParser(sp, "perks", "Get perk count, or add/remove perks");
    ArgChoices(ArgNargs(AddArgument(perks, "action", NULL), NARGS_OPTIONAL), "add", "remove", NULL);
    ArgHelp(ArgNargs(AddArgument(perks, "perks", NULL), NARGS_ZERO_OR_MORE),
            "perk names, e.g. juggernaut quick-revive");

    Parser *weapons = AddParser(sp, "weapons", "Give weapons by name, or 'take' to clear all");
    ArgHelp(ArgNargs(AddArgument(weapons, "weapons", NULL), NARGS_ONE_OR_MORE),
            "weapon names (e.g. ray-gun thundergun), or 'take' to remove all");

    Parser *graphics = AddParser(sp, "graphics", "Graphics config (get, or set KEY=value)");
    ArgHelp(ArgNargs(AddArgument(graphics, "set", NULL), NARGS_ZERO_OR_MORE),
            "assignments, e.g. fov=95 borderless=on");
    AddParser(sp, "customizer", "Customizer config");

    Parser *binds = AddParser(sp, "binds", "Key binds (list, add, rm, reset)");
    ArgChoices(ArgNargs(AddArgument(binds, "action", NULL), NARGS_OPTIONAL),
               "add", "rm", "reset", NULL);
    ArgHelp(ArgNargs(AddArgument(binds, "args", NULL), NARGS_ZERO_OR_MORE),
            "for 'add': KEY=command (e.g. F5=\"/god\"); for 'rm': key names (e.g. F5)");

    AddParser(sp, "music", "Play the easter-egg song");

    Parser *player = AddParser(sp, "player", "Player scalar attributes (get, or set KEY=value)");
    ArgHelp(ArgNargs(AddArgument(player, "set", NULL), NARGS_ZERO_OR_MORE),
            "assignments, e.g. points=50000 health=100 (character: use 'game config')");

    Parser *stats = AddParser(sp, "stats", "Active-game stats (optionally scoped to a round)");
    ArgType(ArgNargs(AddArgument(stats, "round", NULL), NARGS_OPTIONAL), TYPE_INT);

    Parser *trade = AddParser(sp, "trade", "Trade timer");
    Argument *tradeAction = AddArgument(trade, "action", NULL);
    ArgNargs(tradeAction, NARGS_OPTIONAL);
    ArgChoices(tradeAction, "status", "total", "start", "end", "cancel", NULL);
    ArgHelp(tradeAction, "status (default), total, start, end, or cancel");

    Parser *widgets = AddParser(sp, "widgets", "Widget overlays (all, one by name, or set KEY=value)");
    ArgNargs(AddArgument(widgets, "name", NULL), NARGS_OPTIONAL);
    ArgHelp(ArgNargs(AddArgument(widgets, "set", NULL), NARGS_ZERO_OR_MORE),
            "assignments, e.g. enabled=on font=\"Arial\" font-size=48 "
            "color=255,0,0,255 hide-outside-game=off rect=200,200,300,100");
}

int cliMain(int argc, char **argv) {
    Parser *p = NewParser("bo1zt");
    buildParser(p);

    Namespace *ns = ParseArgs(p, argc, argv);
    if (!ns) { FreeParser(p); return 2; }

    const char *command = GetSubcommand(ns);
    const Namespace *sub = GetSubnamespace(ns);
    int rc = 2;

    Client *client = clientCreate(serviceResolvePort());
    if (!client) { FreeNamespace(ns); FreeParser(p); return 1; }

    if (!command) {
        rc = 2;
    } else if (strcmp(command, "health") == 0) {
        rc = cmdHealth(client);
    } else if (strcmp(command, "state") == 0) {
        rc = cmdState(client);
    } else if (strcmp(command, "commands") == 0) {
        rc = cmdCommands(client);
    } else if (strcmp(command, "game") == 0) {
        rc = cmdGame(client, sub);
    } else if (strcmp(command, "cheats") == 0) {
        rc = cmdCheats(client, sub);
    } else if (strcmp(command, "cheat") == 0) {
        rc = cmdCheat(client, sub);
    } else if (strcmp(command, "round") == 0) {
        rc = cmdRound(client, sub);
    } else if (strcmp(command, "position") == 0) {
        rc = cmdPosition(client);
    } else if (strcmp(command, "teleport") == 0) {
        rc = cmdTeleport(client, sub);
    } else if (strcmp(command, "ammo") == 0) {
        ClientResult r = clientGiveAmmo(client);
        rc = (r == CLIENT_OK) ? 0 : reportClientError(client, r);
    } else if (strcmp(command, "perks") == 0) {
        rc = cmdPerks(client, sub);
    } else if (strcmp(command, "weapons") == 0) {
        rc = cmdWeapons(client, sub);
    } else if (strcmp(command, "graphics") == 0) {
        rc = cmdGraphics(client, sub);
    } else if (strcmp(command, "customizer") == 0) {
        rc = cmdCustomizer(client);
    } else if (strcmp(command, "binds") == 0) {
        rc = cmdBinds(client, sub);
    } else if (strcmp(command, "music") == 0) {
        ClientResult r = clientPlayMusic(client);
        rc = (r == CLIENT_OK) ? 0 : reportClientError(client, r);
    } else if (strcmp(command, "widgets") == 0) {
        rc = cmdWidgets(client, sub);
    } else if (strcmp(command, "player") == 0) {
        rc = cmdPlayer(client, sub);
    } else if (strcmp(command, "stats") == 0) {
        rc = cmdStats(client, sub);
    } else if (strcmp(command, "trade") == 0) {
        rc = cmdTrade(client, sub);
    }

    clientDestroy(client);
    FreeNamespace(ns);
    FreeParser(p);
    return rc;
}
