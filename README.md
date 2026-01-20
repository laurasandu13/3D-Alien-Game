
# Alien Artifact Recovery – 3D OpenGL Game
by Ionescu Ionut and Sandu Laura Florentina - 1231IoT

***

Alien Artifact Recovery is a modern OpenGL 3D game built with C++17, GLFW, GLEW and GLM.
The project is the 3D continuation of a 2D teaser: you explore an alien desert, avoid hazardous pits and obstacles, collect artifacts, and escape with your spaceship.

The focus of the project is not on photorealistic art, but on implementing non‑trivial real‑time graphics and gameplay systems: custom camera, procedural terrain, collision detection, hazard zones with lives, and a minimal HUD.

***

## Features

- First‑person camera based on GLM, with mouse‑look and scroll‑wheel FOV zoom.
- Procedurally generated heightfield terrain with textured “alien sand” and carved pits.
- Loading and rendering of textured OBJ models (crates, sphere sun) via custom `Mesh` / `MeshLoaderObj`.
- Axis‑aligned bounding box (AABB) collision between player capsule and world objects.
- Hazard zones (pits) that trigger a fall animation, life loss, and respawn.
- Simple 2D HUD overlay for displaying remaining lives (hearts) rendered with a dedicated HUD shader.
- Modern OpenGL only: VAO, VBO, IBO, programmable pipeline, no deprecated fixed‑function calls.

***

## Technologies

- **Language:** C++17
- **Graphics API:** OpenGL 4.x (Core Profile)
- **Window / Input:** GLFW
- **Extension Loading:** GLEW
- **Math:** GLM (vectors, matrices, camera math, projection)
- **Model Loading:** Custom OBJ loader (`MeshLoaderObj`) into a `Mesh` abstraction with VAO/VBO/IBO.
- **Textures:** BMP loading and OpenGL texture objects.

***

## Project Structure

- `Graphics/window.h` – window wrapper around GLFW, handling input polling and buffer swapping.
- `Camera/camera.h` – FPS‑style camera with view direction, movement, and yaw/pitch updates.
- `Shaders/shader.h` – shader compilation/linking, uniform utilities.
- `Model Loading/mesh.h` – generic mesh storing vertices, indices, VAO/VBO/IBO and a `draw()` method.
- `Model Loading/meshLoaderObj.h` – OBJ loader creating `Mesh` instances with textures.
- `Model Loading/texture.h` – helpers for loading BMPs and creating OpenGL textures.
- `GameState.h` – game state container: hazard zones, tasks, and other world logic.
- `main.cpp` – entry point: initialization, terrain generation, object placement, game loop, and HUD.

***

## Core Systems

### First‑Person Camera

The player is represented by a camera positioned in world space:

- Camera position is a `glm::vec3` updated each frame from keyboard input.
- Mouse movement controls yaw and pitch; a direction vector is constructed and normalized, and passed to `Camera::setCameraViewDirection`.
- Scroll wheel adjusts the FOV used in `glm::perspective`, allowing zoom in/out without touching the world matrices.

This matches the standard FPS camera from LearnOpenGL (position, front, up, yaw, pitch, FOV) but is implemented in a custom `Camera` class.

***

### Procedural Terrain with Pits

Terrain is generated procedurally in `createTerrainMesh`:

- A regular grid of `(divisions + 1)²` vertices is created over a square area.
- Base height is a sum of trigonometric functions in X and Z to simulate dunes.
- For each hazard zone, vertices within a pit radius are moved down with a smooth falloff (`distance / radius`, squared) to carve a depression.
- Normals are computed per vertex from finite differences (using neighboring vertices) to get correct lighting.
- Indices are generated so each grid cell becomes two triangles.

The result is a single `Mesh` with a sand texture that already includes the pits as real geometry, not just a texture trick.

***

### Mesh and OBJ Loading

Static objects (crates, sphere sun) are imported from OBJ files:

- `MeshLoaderObj` parses positions, normals, and texture coordinates from OBJ.
- A vector of `Vertex` and index data is built and uploaded into a `Mesh`.
- The `Mesh` creates a VAO, VBO and EBO; attributes (position, normal, texcoord) are enabled at fixed locations.
- Multiple `Mesh` instances share textures via a `Texture` array; each instance is placed in the world with its own position, scale and rotation angle (stored in `ObjectInstance`).

This explicit separation between mesh, transform and texture is critical to reusing assets and staying within modern OpenGL best practices.

***

### Player Collision and Movement

The player is approximated by a standing capsule:

- Horizontal size: radius in XZ (`PLAYER_RADIUS`).
- Vertical size: height (`PLAYER_HEIGHT`).

Collision with static objects is handled with axis‑aligned bounding boxes:

- Player AABB is computed around the camera position.
- Each crate instance defines its own AABB from `ObjectInstance.position` and `scale`.
- `checkCollision3D` tests overlap in X, Y and Z to detect intersection.
- `checkCollisionAllowJump` ignores collisions when the player’s feet are above the object top, allowing jumping over low objects.
- On movement input (W/A/S/D), the new camera position is tentatively applied; if collision is detected, the move is reverted.

This approach provides robust, easy‑to‑debug collision suitable for a first‑person game without complex physics.

***

### Jump, Crouch and Ground Handling

Vertical movement is handled independently from horizontal:

- `verticalVelocity` and a simple constant acceleration `GRAVITY` implement a 1D physics integration along Y.
- Pressing space while grounded sets `verticalVelocity = JUMP_SPEED`, and gravity then pulls the player back down.
- Ground level is switched between `STAND_HEIGHT` and `CROUCH_HEIGHT` depending on whether crouch is active.
- The camera’s Y is clamped to the active ground level to prevent underground tunneling.

This yields predictable jumping, crouching and stable grounding over the procedural terrain baseline.

***

### Hazard Zones, Falling and Lives

Hazard pits are represented by a `HazardZone` struct stored in `GameState`:

- Each zone has a center `position`, a `size` (half‑extents in X and Z), a damage value and a description string.
- At runtime, the player’s XZ position is tested against each zone’s bounds. If inside, a fall is triggered.
- During fall:
    - Movement input is disabled.
    - `verticalVelocity` is set downward, and the camera position is updated each frame to animate falling.
    - After `fallDuration` seconds, a life is removed and the player is respawned at a safe `respawnPoint`.

The game tracks `maxLives` and `lives`; when `lives` reaches zero, the loop terminates and the game ends. This entire pipeline reuses the jump/physics logic but with dynamics modified for a faster, more dramatic drop.

***

### 2D HUD Hearts Overlay

Lives are represented as hearts in the top‑right corner of the screen:

- A dedicated HUD shader pair (`hud_vertex_shader.glsl` / `hud_fragment_shader.glsl`) is used.
- The HUD quad is a simple 0..1 square in local space; it is rendered in an orthographic projection over window pixel coordinates (`glm::ortho(0, width, 0, height)`).
- For each life slot, a model matrix translates the quad to the correct screen position and scales it to the desired pixel size.
- Hearts corresponding to remaining lives are drawn in bright red; lost lives are drawn in darker red.

Because the HUD uses its own shader, VAO and orthographic projection, it does not interfere with the 3D pipeline and always renders on top by disabling depth testing for the HUD pass.

***

## Building and Running

1. Clone the repository:
`git clone https://github.com/laurasandu13/3D-Alien-Game.git`
2. Make sure you have:
    - A C++17 compiler (Visual Studio, Clang or GCC).
    - GLFW, GLEW and GLM installed or included as dependencies.
3. Configure the project in your IDE or with CMake to link against GLFW and GLEW and to include GLM headers.
4. Build and run.
5. Controls:
    - `W / A / S / D` – move forward / left / back / right.
    - `Mouse` – look around.
    - `Scroll wheel` – change FOV (zoom).
    - `Space` – jump.
    - `Ctrl` – crouch.
    - `Esc` – exit.

***

## Future Work

Planned extensions that build on this foundation:

- Reuse the hazard and health systems from the 2D teaser in a richer 3D level.
- Add tasks (artifact collection, pylon activation, tower repair, final ship escape) with proximity‑based interaction and state machine logic.
- Integrate a full GUI library (e.g., Dear ImGui) for task display, artifact counters, and restart button.
- Introduce a scene graph for parent–child relationships (artifacts on pedestals, animated pylons) and hierarchical transforms.
- Improve lighting and atmosphere to enhance the alien environment.

This project is primarily a study in building a small, coherent modern OpenGL “engine” around a concrete gameplay idea, rather than a content‑heavy commercial game.
