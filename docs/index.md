# CS184 Project Progress Report

Toby Chen, Bob Cao, Gui Andrade

## Project Architecture & expectations

![](https://media.discordapp.net/attachments/561306007385669655/573028789488058368/Final184Architecture.png?width=400&height=267)

(TODO: Replace the link with a permanent one)

## Implemented components

- Vulkan rendering pipeline
- Scene graph with model rendering
    - Implemented GLTF loading for easy importation of complex scenes
    - Using an [octree](https://en.wikipedia.org/wiki/Octree) as our internal data structure for high performance at scale
- Horizon Based Ambient Occlusion (HBAO) shading
- Interactivity
    - Using SDL2 for window management and mouse/keyboard input
    - First-person camera point of view

## Progress

All the basic infrastructures that make the project code base scalable and not constrained to this single project are mostly done. We are still in the process of implementing the full deferred rendering pipeline and the ambient occlusion pipeline. Shadow mapping, PBR based lighting, voxelization, and cone tracing has not been started yet.

The project is a bit behind the original timeline, but we think it is benefitial to have a robust infrastructure in place before we start implementing complex algorithms as those are prone to subtle errors.

## Next steps

- Complete Megapipeline
- Voxel LOD volumes generation
- Shadow mapping (cascaded shadow mapping if possible)
- Ground Truth Ambient Occlusion (GTAO) (adapt from existing HBAO code)
- Voxel marching
