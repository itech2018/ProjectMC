# ProjectMC GPU shaders

These HLSL sources are the canonical voxel shaders.

The runtime loads compiled shader binaries rather than compiling shaders while
the game is running. This keeps the shipping game independent of a shader
compiler and lets us package DXIL for Direct3D, SPIR-V for Vulkan, and MSL for
Metal.

## Resource layout

Vertex shader:
- uniform buffer 0: camera view-projection matrix

Fragment shader:
- sampler/texture 0: block texture atlas

## Windows development

When `SDL_shadercross` is available, the CMake `projectmc-shaders` target
can be used to produce backend shader binaries. Normal ProjectMC builds do not
require shadercross yet; this avoids breaking compatibility-renderer builds
during the GPU migration.
