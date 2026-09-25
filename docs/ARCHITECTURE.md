# ProjectMC Architecture

## Principles

1. Original implementation and assets.
2. Data-driven blocks, items, recipes and content.
3. Authoritative dedicated-server architecture.
4. Chunk-based worlds with asynchronous generation and streaming.
5. Fluids, inventories, energy and machine capabilities are first-class systems.
6. Modding and resource/data packs remain first-class even though many modpack-style features ship in core.
7. The launcher supports side-by-side versions.

## Components

**Game:** rendering, input, UI, audio, client prediction and presentation.

**Shared:** versioning and later identifiers, serialization, protocol definitions and common simulation primitives.

**Server:** authoritative world simulation, players, entities, permissions and networking.

**Launcher:** installation profiles, release channels, manifests, updates, integrity checking and mods.

**Services:** optional online accounts, signed manifests and server discovery.

## World direction

World -> regions -> chunks -> sections -> block/state storage.

Exact dimensions and serialization are deferred until voxel foundations so prototypes do not accidentally become permanent save-format constraints.
