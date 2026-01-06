<div style="display:flex;">
  <img src="res/icon.png" alt="bo1zt icon" width="128">
</div>

# bo1zt

[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/idircarlos/bo1zt)
[![GitHub stars](https://img.shields.io/github/stars/idircarlos/bo1zt?style=social)](https://github.com/idircarlos/bo1zt/stargazers)
[![GitHub forks](https://img.shields.io/github/forks/idircarlos/bo1zt?style=social)](https://github.com/idircarlos/bo1zt/network/members)
[![GitHub issues](https://img.shields.io/github/issues/idircarlos/bo1zt)](https://github.com/idircarlos/bo1zt/issues)
[![GitHub license](https://img.shields.io/github/license/idircarlos/bo1zt)](https://github.com/idircarlos/bo1zt/blob/main/LICENSE)
[![GitHub last commit](https://img.shields.io/github/last-commit/idircarlos/bo1zt)](https://github.com/idircarlos/bo1zt/commits/main)
[![C](https://img.shields.io/badge/Language-C-blue)](https://en.wikipedia.org/wiki/C_(programming_language))
[![libui](https://img.shields.io/badge/Library-libui-blue)](https://github.com/libui-ng/libui-ng)
[![OpenGL](https://img.shields.io/badge/Library-OpenGL-blue)](https://www.opengl.org/)
[![iniparser](https://img.shields.io/badge/Library-iniparser-blue)](https://gitlab.com/iniparser/iniparser)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen)](https://github.com/idircarlos/bo1zt/fork)

**Black Ops 1 Zombies Trainer**

A trainer for *Black Ops 1 Zombies* written in C.
This tool is being created just for fun while learning reverse engineering fundamentals. This tool is still under development. Main features:

|    Player       |  Hacks                 |  Graphics                   |  Misc                                  |
| --------------- | ---------------------- | --------------------------- | -------------------------------------- |
|   Change name   |   God Mode             |   Set FOV                   |   Give weapons                         |
|   Set Health    |   No Clip              |   Set FOV Scale             |   Give ammo                            |
|   Set Points    |   Invisible            |   Set FPS Cap / Unlimit FPS |   Teleport to any location (save/load) |
|   Set Speed     |   No Recoil            |   Make Borderless           |   Change to any round                  |
|   Set Kills     |   Infinite Ammo        |   Disable HUD               |   Play easter egg music any time       |
|   Set Headshots |   Box Never Moves      |   Disable FOG               |   Fix Movement Speed PC Issue          |
|                 |   Instant Kill         |   Colorized mode            |   Show FPS                             |
|                 |   Small Crosshair      |   Fullbright mode           |   Commands                             |
|                 |   Fast Gameplay        |   Customize UI              |   Bind Manager                         |
|                 |   Third Person         |                             |   Character Selection                  |
|                 |   No Shellshock        |                             |   Add Floating Widgets                 |
|                 |   Increase Knife Range |                             |   Open/Close Game                      |
|                 |                        |                             |   Game resets                          |
|                 |                        |                             |   Persisted Settings                   |


## Differences between others mods
This tool allows injecting any GSC script into the game without using any external mod tool. Because of this, I was able to set up bidirectional communication between bo1zt and GSC scripts via DVars (bo1zt → GSC) and VM notifications (GSC → bo1zt).

This is extremely powerful, since the mod can be extended to do whatever your imagination can come up with, without relying on all the limitations that GSC has. To explain it in a simpler way, you could say that you can ask GSC to do whatever you want from this mod at any moment, programmatically.

Hypothetical examples that are not implemented:
- Play the Easter egg song when your favorite streamer starts streaming.
- Give a random perk when your friend sends you a Discord message.
- Use Dempsey, Nikolai, Takeo, or Richtofen depending on the day of the week you are playing.

I hope this makes sense.

## Requirements

To build and run this project, you’ll need:

* **MinGW-32** - This is for the `g++` compiler and `make`.
* **Meson** (version **0.58.0** - I didn't test it with newer versions) - required to build the [libui]([https://github.com/libui-ng/libui-ng](https://github.com/libui-ng/libui-ng)) library.
* **CMake** (version **3.18.0** - I didn't test it with newer versions) - required to build the [iniparser]([https://github.com/libui-ng/libui-ng](https://gitlab.com/iniparser/iniparser)) library.

## Build and Run

```bash
make -j8
make run
```

## Motivation
There are two main reasons why I created this trainer:
- To learn basic concepts of reverse engineering while enjoying one of my favorite games from my childhood.
- Out of curiosity about how [TIM](https://download.magicbennie.com/BlackOpsZombies/TIM/) works under the hood, since it wasn’t open source.

If you have used TIM before, you will definitely find some similarities, but this trainer has its own unique features as well.

My intention in releasing this source code is not to “reveal how to hack BO1,” since there are already plenty of existing trainers out there that already show it, but rather to share how things actually work, how I managed to implement some very unique features, and how customizable this can be in the long term.

After using TIM, it was really nice to have the FOV and some widgets automatically configured, but I felt that some other features were missing. Because of that, I decided to implement things such as:
- Persistent UI color configuration regardless of the game language
- Automatic power-up cycle widget
- Selecting which character I want to use for every run, so I don’t need to restart so many times
- Being able to start at round 40 with any loadout I want, to train any strategy in a fast manner
- Integrated borderless mode, since I was previously using an external application for this
- Adding my own commands
- Adding specific mod commands to key bindings instead of native BO1 commands

Note: If someone is willing to hack a game with malicious intentions, such as faking speedruns, they will do it anyway, whether with this tool or any other.

## Acknowledgments
I would like to extend my gratitude to:
- [magicbennie](https://github.com/magicbennie), creator of TIM, for being an inspiration while creating this tool.
- [Lunar RF Labds](https://lunar.sh), from whom I took the idea of GSC loading based on [one of their journal entries](https://journal.lunar.sh/2023/gsctool.html).