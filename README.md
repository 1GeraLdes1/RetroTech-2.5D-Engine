# RetroTech 2.5D Engine

**RetroTech** is an experimental 2.5D software raycasting engine written in C++.

The project is built as a low-level learning engine inspired by classic pseudo-3D games. It does not use Unity, Unreal Engine, SFML, SDL, or olcPixelGameEngine. The engine uses its own small WinAPI-based pixel backend called **CustomPixelEngine**.

## Features

* 2.5D raycasting renderer
* Custom WinAPI-based pixel engine
* Software framebuffer rendering
* Manual pixel drawing
* Procedural wall texture
* Distance fog
* WASD player movement
* Mouse look
* Mouse capture
* Wall collision
* Minimap
* FPS counter in the window title
* Optimized wall rendering
* No external game engine dependency

## CustomPixelEngine

RetroTech uses a custom pixel backend instead of an external game framework.

`CustomPixelEngine` handles:

* window creation through WinAPI
* software framebuffer
* pixel output through `StretchDIBits`
* drawing primitives
* keyboard input
* mouse capture
* mouse look support
* FPS counter
* main game loop

The raycasting renderer itself is implemented inside the RetroTech engine code.

## How It Works

The engine uses raycasting to create a 2.5D view.

For every vertical column of the screen, the engine casts a ray from the player position into the map. The ray moves through the grid until it hits a wall. After that, the engine calculates the distance to the wall and draws a vertical textured column on the screen.

Closer walls are drawn taller, while distant walls are drawn shorter. This creates a pseudo-3D perspective.

The map is currently stored as a simple grid:

* `1` means wall
* `0` means empty space

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
| ESC               | Exit                 |

## Project Structure

```text
RetroTech/
 ├── CustomPixelEngine.h
 ├── RetroTech.cpp
 ├── README.md
 └── .gitignore
```

## Building

Open the project in Visual Studio and build it as a C++ Windows application.

The project currently targets Windows because it uses WinAPI directly.

Required files:

```text
CustomPixelEngine.h
RetroTech.cpp
```

## Current Status

RetroTech is an early prototype.

The current version includes a basic raycasting renderer, player movement, collision, mouse look, a minimap, a custom pixel backend, and basic rendering optimization.

## Planned Features

* Map loading from external files
* Multiple wall types
* Doors
* Sprites
* Enemies
* Weapons
* Floor and ceiling textures
* Level editor
* Better renderer structure
* More advanced optimization

## License

No license has been selected for RetroTech yet.

All rights to the RetroTech source code are reserved unless a license is added later.
