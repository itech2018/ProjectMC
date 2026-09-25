# Voxel Foundations

ProjectMC begins with 16x16x16 chunk sections. This is an internal storage unit rather than a final world-height restriction.

Blocks use stable numeric runtime IDs backed by a registry. Air is always ID 0. Definitions carry gameplay properties such as material, solidity and transparency; visual state will be expanded separately.

The first test world produces a flat stone/dirt/grass chunk. This deliberately separates world storage from rendering so the renderer can later consume chunk meshes without owning simulation data.

## Next
- block states/properties
- mesh vertex format
- visible-face generation
- 3D camera matrices
- GPU chunk buffers
- block placement/removal
