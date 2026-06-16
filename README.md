# RetroTech 2.5D Engine

**RetroTech** is an experimental 2.5D software raycasting engine written in C++.

The project is designed as a low-level learning engine inspired by old pseudo-3D games. Wall rendering is done through raycasting, without Unity, Unreal Engine, SFML, or any ready-made 3D engine.

## Features

* 2.5D raycasting renderer
* Software wall rendering
* Procedural wall texture
* Distance fog
* WASD player movement
* Mouse look
* Mouse capture
* Wall collision
* Minimap
* Optimized wall rendering
* Single `.cpp` source file

## Controls

| Key               | Action               |
| ----------------- | -------------------- |
| W                 | Move forward         |
| S                 | Move backward        |
| A                 | Move left            |
| D                 | Move right           |
| Shift             | Run                  |
| Mouse             | Look around          |
| Left Mouse Button | Capture mouse        |
| F2                | Toggle mouse capture |
| TAB               | Show minimap         |
| F1                | Show help            |
| ESC               | Exit                 |

## Dependencies

This project uses:

* C++
* Visual Studio
* Windows API
* olcPixelGameEngine

The file `olcPixelGameEngine.h` must be placed next to the main source file.

## Building

Open the `.sln` file in Visual Studio and build the project.

Alternatively, create a new C++ project and add:

```text
main.cpp
olcPixelGameEngine.h
```

## Project Status

This project is in an early prototype stage.

The current version is a basic 2.5D engine prototype with raycasting rendering, player movement, mouse look, wall collision, and basic wall-rendering optimization.

## Planned Features

* Map loading from external files
* Multiple wall types
* Doors
* Sprites
* Enemies
* Weapons
* Floor and ceiling textures
* Level editor
* More advanced rendering optimization

## Credits

This project uses **olcPixelGameEngine** by OneLoneCoder.

olcPixelGameEngine is licensed separately under the OLC-3 license.

## License

No license has been selected for RetroTech yet.

All rights to the RetroTech source code are reserved unless a license is added later.

