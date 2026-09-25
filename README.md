# ProjectMC

ProjectMC is an original voxel sandbox focused on exploration, building, survival, engineering, automation and extensibility.

> ProjectMC is not affiliated with Mojang or Microsoft and does not contain Minecraft source code or assets.

## Repository layout

- `game/` — desktop game client
- `server/` — authoritative dedicated server
- `shared/` — shared game/network code
- `launcher/` — launcher and version manager
- `services/` — optional platform services
- `tools/` — development utilities
- `docs/` — architecture and roadmap

## Current milestone

**v0.0.1 — Project Bootstrap**

Initial implementation uses C++20 and CMake.

## Build

```bash
cmake -S . -B build
cmake --build build
```

See `docs/ROADMAP.md` for the development plan.
