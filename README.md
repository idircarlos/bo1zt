<div style="display:flex;">
  <img src="doc/icon.png" alt="bo1zt icon" width="128">
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
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-brightgreen)](http://makeapullrequest.com)

**Black Ops 1 Zombies Trainer**

A simple trainer for *Black Ops 1 Zombies* written in C.
This tool is being created just for fun while learning reverse engineering fundamentals. This tool is still under development. Main features:

|    Player       |  Hacks                 |  Graphics                   |  Misc                                  |
| --------------- | ---------------------- | --------------------------- | -------------------------------------- |
|   Change name   |   God Mode             |   Set FOV                   |   Give any weapon in any slot          |
|   Set Health    |   No Clip              |   Set FOV Scale             |   Give ammo (*soon!*)                  |
|   Set Points    |   Invisible            |   Set FPS Cap / Unlimit FPS |   Teleport to any location (save/load) |
|   Set Speed     |   No Recoil            |   Make Borderless           |   Change to any round                  |
|   Set Kills     |   Infinite Ammo        |   Disable HUD               |   Game info                            |
|   Set Headshots |   Box Never Moves      |   Disable FOG               |   Fix Movement Speed PC Issue          |
|                 |   Instant Kill         |   Fullbright mode           |   Show FPS                             |
|                 |   Fast Gameplay        |   Colorized mode            |   TIM Compatibility                    |
|                 |   Third Person         |   Customize UI              |   Setup Camos (*soon!*)                 |
|                 |   No Shellshock        |                             |   Add Floating Widgets                 |
|                 |   Increase Knife Range |                             |   Open/Close Game                      |
|                 |   Small Crosshair      |                             |   Persisted Settings                   |

## Requirements

To build and run this project, you’ll need:

* **MinGW-32** - This is for the `g++` compiler and `make`.
* **Meson** (version **0.58.0** - I didn't test it with newer versions) - required to build the [libui]([https://github.com/libui-ng/libui-ng](https://github.com/libui-ng/libui-ng)) library.
* **CMake** (version **3.18.0** - I didn't test it with newer versions) - required to build the [iniparser]([https://github.com/libui-ng/libui-ng](https://gitlab.com/iniparser/iniparser)) library.

## Build Instructions

```bash
make -j8 && make run
```
