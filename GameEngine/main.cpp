#include "Graphics/window.h"
#include "Camera/camera.h"
#include "Shaders/shader.h"
#include "Model Loading/mesh.h"
#include "Model Loading/texture.h"
#include "Model Loading/meshLoaderObj.h"
#include "GameState.h"
#include <iostream>
#include <vector>

// Function declarations
void processKeyboardInput();

// Global variables
float deltaTime = 0.0f;
float lastFrame = 0.0f;

Window window("Alien Artifact Recovery", 1280, 720);
Camera camera(glm::vec3(0.0f, 10.0f, 780.0f));

GameState gameState;

glm::vec3 lightColor = glm::vec3(1.0f);
glm::vec3 lightPos = glm::vec3(0.0f, 40.0f, 0.0f);

// Track E key press
bool eKeyPressed = false;
bool eKeyWasPressed = false;

// Structure to hold object instance data (position, scale, rotation)
struct ObjectInstance {
    Mesh* mesh;
    glm::vec3 position;
    glm::vec3 scale;
    float rotationY;
};

// Create object instances
std::vector<ObjectInstance> objects;

// Check if a point (player position) collides with an object's bounding box
bool checkCollision(glm::vec3 playerPos, glm::vec3 objectPos, glm::vec3 objectScale, float playerRadius) {
    float halfSizeX = objectScale.x + playerRadius;
    float halfSizeZ = objectScale.z + playerRadius;

    if (playerPos.x >= objectPos.x - halfSizeX && playerPos.x <= objectPos.x + halfSizeX &&
        playerPos.z >= objectPos.z - halfSizeZ && playerPos.z <= objectPos.z + halfSizeZ) {
        return true;
    }

    return false;
}

// Check collision with all objects in the scene
bool checkAllCollisions(glm::vec3 newPosition, const std::vector<ObjectInstance>& objects, float playerRadius) {
    for (const auto& obj : objects) {
        if (checkCollision(newPosition, obj.position, obj.scale, playerRadius)) {
            return true;
        }
    }
    return false;
}

// Function to create terrain with hills/dunes and carved pits
Mesh createTerrainMesh(float size, int divisions, GLuint textureID, const std::vector<HazardZone>& pits) {
    std::vector<Vertex> vertices;
    std::vector<int> indices;
    std::vector<Texture> texVec;

    for (int z = 0; z <= divisions; ++z) {
        for (int x = 0; x <= divisions; ++x) {
            float fx = -size + 2.0f * size * (float)x / (float)divisions;
            float fz = -size + 2.0f * size * (float)z / (float)divisions;

            Vertex v;

            // Create hills/dunes using sine waves
            float height = 0.5f * sin(fx * 0.05f) * cos(fz * 0.05f)
                + 0.3f * sin(fx * 0.15f) * cos(fz * 0.15f)
                + 0.2f * sin(fx * 0.3f) * cos(fz * 0.3f);

            // Check if this vertex is inside any pit zone and carve pit
            for (const auto& pit : pits) {
                float dx = fx - pit.position.x;
                float dz = fz - pit.position.z;
                float distance = sqrt(dx * dx + dz * dz);
                float pitRadius = pit.size.x / 2.0f; // Use X size as radius

                // If inside pit radius, create depression
                if (distance < pitRadius) {
                    // Smooth falloff - deeper in center, slopes at edges
                    float normalizedDist = distance / pitRadius; // 0 at center, 1 at edge
                    float falloff = 1.0f - normalizedDist; // 1 at center, 0 at edge
                    falloff = falloff * falloff; // Square for smoother curve

                    // Depress terrain (subtract from current height)
                    float pitDepth = 4.0f; // How deep the pit is (increased for visibility on hills)
                    height -= pitDepth * falloff; // Subtract to create depression
                }
            }

            v.pos = glm::vec3(fx, height, fz);

            v.textureCoords = glm::vec2(
                (float)x / (float)divisions * 10.0f,
                (float)z / (float)divisions * 10.0f
            );

            vertices.push_back(v);
        }
    }

    // Calculate proper normals based on actual terrain shape
    for (int z = 0; z <= divisions; ++z) {
        for (int x = 0; x <= divisions; ++x) {
            int idx = z * (divisions + 1) + x;

            // Get neighboring positions for normal calculation
            glm::vec3 pos = vertices[idx].pos;

            glm::vec3 posLeft = (x > 0) ? vertices[idx - 1].pos : pos;
            glm::vec3 posRight = (x < divisions) ? vertices[idx + 1].pos : pos;
            glm::vec3 posDown = (z > 0) ? vertices[idx - (divisions + 1)].pos : pos;
            glm::vec3 posUp = (z < divisions) ? vertices[idx + (divisions + 1)].pos : pos;

            // Calculate tangent vectors
            glm::vec3 tangentX = glm::normalize(posRight - posLeft);
            glm::vec3 tangentZ = glm::normalize(posUp - posDown);

            // Cross product gives normal
            glm::vec3 normal = glm::normalize(glm::cross(tangentZ, tangentX));
            vertices[idx].normals = normal;
        }
    }

    // Create indices for triangles
    for (int z = 0; z < divisions; ++z) {
        for (int x = 0; x < divisions; ++x) {
            int row1 = z * (divisions + 1);
            int row2 = (z + 1) * (divisions + 1);

            int i0 = row1 + x;
            int i1 = row1 + x + 1;
            int i2 = row2 + x;
            int i3 = row2 + x + 1;

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i1);

            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    texVec.push_back(Texture());
    texVec[0].id = textureID;
    texVec[0].type = "texture_diffuse";

    Mesh terrainMesh(vertices, indices, texVec);
    return terrainMesh;
}

int main()
{
    glClearColor(0.4f, 0.6f, 0.8f, 1.0f);

    // Building and compiling shader program
    Shader shader("Shaders/vertex_shader.glsl", "Shaders/fragment_shader.glsl");
    Shader sunShader("Shaders/sun_vertex_shader.glsl", "Shaders/sun_fragment_shader.glsl");

    // Load textures
    GLuint sandTex = loadBMP("Resources/Textures/sand.bmp");
    GLuint crate1x1Tex = loadBMP("Resources/Models/StaticObjects/crates/Crate_1x1_Mat_BaseColor.bmp");
    GLuint crate1x1TallTex = loadBMP("Resources/Models/StaticObjects/crates/Crate_1x1_Tall_Mat_BaseColor.bmp");
    GLuint crate1x2Tex = loadBMP("Resources/Models/StaticObjects/crates/Crate_1x2_Mat_BaseColor.bmp");
    GLuint crate1x2TallTex = loadBMP("Resources/Models/StaticObjects/crates/Crate_1x2_Tall_Mat_BaseColor.bmp");
    GLuint crate2x2TallTex = loadBMP("Resources/Models/StaticObjects/crates/Crate_2x2_Tall_Mat_BaseColor.bmp");

    // Create texture vectors
    std::vector<Texture> crate1x1TexVec;
    crate1x1TexVec.push_back(Texture());
    crate1x1TexVec[0].id = crate1x1Tex;
    crate1x1TexVec[0].type = "texture_diffuse";

    std::vector<Texture> crate1x1TallTexVec;
    crate1x1TallTexVec.push_back(Texture());
    crate1x1TallTexVec[0].id = crate1x1TallTex;
    crate1x1TallTexVec[0].type = "texture_diffuse";

    std::vector<Texture> crate1x2TexVec;
    crate1x2TexVec.push_back(Texture());
    crate1x2TexVec[0].id = crate1x2Tex;
    crate1x2TexVec[0].type = "texture_diffuse";

    std::vector<Texture> crate1x2TallTexVec;
    crate1x2TallTexVec.push_back(Texture());
    crate1x2TallTexVec[0].id = crate1x2TallTex;
    crate1x2TallTexVec[0].type = "texture_diffuse";

    std::vector<Texture> crate2x2TallTexVec;
    crate2x2TallTexVec.push_back(Texture());
    crate2x2TallTexVec[0].id = crate2x2TallTex;
    crate2x2TallTexVec[0].type = "texture_diffuse";

    glEnable(GL_DEPTH_TEST);

    MeshLoaderObj loader;
    Mesh sun = loader.loadObj("Resources/Models/sphere.obj");

    // Load static objects
    std::vector<Mesh> staticMeshes;

    std::cout << "Loading static objects..." << std::endl;

    staticMeshes.push_back(loader.loadObj("Resources/Models/StaticObjects/crates/Crate_1x1.obj", crate1x1TexVec));
    staticMeshes.push_back(loader.loadObj("Resources/Models/StaticObjects/crates/Crate_1x1_Tall.obj", crate1x1TallTexVec));
    staticMeshes.push_back(loader.loadObj("Resources/Models/StaticObjects/crates/Crate_1x2.obj", crate1x2TexVec));
    staticMeshes.push_back(loader.loadObj("Resources/Models/StaticObjects/crates/Crate_1x2_Tall.obj", crate1x2TallTexVec));
    staticMeshes.push_back(loader.loadObj("Resources/Models/StaticObjects/crates/Crate_2x2_Tall.obj", crate2x2TallTexVec));

    std::cout << "Loaded " << staticMeshes.size() << " static object types." << std::endl;

    // Scatter crates around (regular obstacles)
    objects.push_back({ &staticMeshes[0], glm::vec3(-80, 0, -80), glm::vec3(7.5, 7.5, 7.5), 0 });
    objects.push_back({ &staticMeshes[1], glm::vec3(-100, 0, -60), glm::vec3(7.5, 7.5, 7.5), 30 });
    objects.push_back({ &staticMeshes[2], glm::vec3(-120, 0, -70), glm::vec3(7.5, 7.5, 7.5), 60 });
    objects.push_back({ &staticMeshes[3], glm::vec3(150, 0, 120), glm::vec3(9.0, 9.0, 9.0), 90 });
    objects.push_back({ &staticMeshes[4], glm::vec3(170, 0, 140), glm::vec3(10.0, 10.0, 10.0), 120 });
    objects.push_back({ &staticMeshes[0], glm::vec3(-200, 0, 100), glm::vec3(6.0, 6.0, 6.0), 150 });
    objects.push_back({ &staticMeshes[2], glm::vec3(250, 0, -150), glm::vec3(8.0, 8.0, 8.0), 180 });
    objects.push_back({ &staticMeshes[1], glm::vec3(50, 0, 200), glm::vec3(7.0, 7.0, 7.0), 210 });
    objects.push_back({ &staticMeshes[3], glm::vec3(-300, 0, -50), glm::vec3(8.5, 8.5, 8.5), 240 });
    objects.push_back({ &staticMeshes[4], glm::vec3(300, 0, -200), glm::vec3(11.0, 11.0, 11.0), 270 });

    std::cout << "Placed " << objects.size() << " object instances in the world." << std::endl;

    // Create hazard zones before creating terrain (so we can carve pits)
    std::cout << "\nCreating hazard pits..." << std::endl;

    // Scattered pits around the map (smaller size: 15-25 units)
    gameState.addHazardZone(glm::vec3(50, 0, 50), glm::vec3(18, 10, 18), 1, "Radiation Pit Alpha");
    gameState.addHazardZone(glm::vec3(-150, 0, -100), glm::vec3(22, 10, 22), 1, "Toxic Pit Beta");
    gameState.addHazardZone(glm::vec3(200, 0, 150), glm::vec3(20, 10, 20), 1, "Crater Gamma");
    gameState.addHazardZone(glm::vec3(-250, 0, 200), glm::vec3(25, 10, 25), 1, "Deep Pit Delta");
    gameState.addHazardZone(glm::vec3(180, 0, -120), glm::vec3(15, 10, 15), 1, "Small Pit Epsilon");
    gameState.addHazardZone(glm::vec3(-80, 0, 250), glm::vec3(18, 10, 18), 1, "Hazard Pit Zeta");
    gameState.addHazardZone(glm::vec3(300, 0, 50), glm::vec3(23, 10, 23), 1, "Alien Crater Eta");
    gameState.addHazardZone(glm::vec3(-200, 0, -200), glm::vec3(17, 10, 17), 1, "Dark Pit Theta");
    gameState.addHazardZone(glm::vec3(100, 0, 300), glm::vec3(20, 10, 20), 1, "Danger Zone Iota");

    std::cout << "Created " << gameState.getHazardZones().size() << " hazard pits." << std::endl;

    std::cout << "Generating terrain with sand dunes and carved pits..." << std::endl;
    Mesh terrain = createTerrainMesh(1000.0f, 500, sandTex, gameState.getHazardZones());
    std::cout << "Terrain generated with dunes and pits!" << std::endl;

    // Print initial game state
    std::cout << "\n========================================" << std::endl;
    std::cout << "  ALIEN ARTIFACT RECOVERY MISSION" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Current Task: " << gameState.getCurrentTaskDescription() << std::endl;
    std::cout << "Health: " << gameState.getPlayerHealth() << "/" << gameState.getMaxHealth() << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nControls:" << std::endl;
    std::cout << "  WASD - Move" << std::endl;
    std::cout << "  Arrow Keys - Look around" << std::endl;
    std::cout << "  R/F - Move up/down" << std::endl;
    std::cout << "  E - Interact (when near objects)" << std::endl;
    std::cout << "  ESC - Quit" << std::endl;
    std::cout << "\nHazard Zones:" << std::endl;
    std::cout << "  Look for DARK CRATER PITS carved into the dunes!" << std::endl;
    std::cout << "  Walking into them will damage you." << std::endl;
    std::cout << "  Health regenerates when you're safe." << std::endl;
    std::cout << "========================================\n" << std::endl;

    // Main game loop
    while (!window.isPressed(GLFW_KEY_ESCAPE) &&
        glfwWindowShouldClose(window.getWindow()) == 0 &&
        !gameState.isGameOver())
    {
        window.clear();

        // Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processKeyboardInput();

        // Update health system
        gameState.updateHealthSystem(camera.getCameraPosition(), currentFrame);

        // Check E key
        eKeyWasPressed = eKeyPressed;
        eKeyPressed = window.isPressed(GLFW_KEY_E);

        if (eKeyPressed && !eKeyWasPressed) {
            std::cout << "E key pressed! (Interaction will be added in Phase 6)" << std::endl;
        }

        // Setup matrices
        glm::mat4 ProjectionMatrix = glm::perspective(
            glm::radians(90.0f),
            window.getWidth() * 1.0f / window.getHeight(),
            0.1f,
            10000.0f
        );
        glm::mat4 ViewMatrix = glm::lookAt(
            camera.getCameraPosition(),
            camera.getCameraPosition() + camera.getCameraViewDirection(),
            camera.getCameraUp()
        );

        glBindTexture(GL_TEXTURE_2D, 0);

        //// Render sun ////
        sunShader.use();
        GLuint MatrixID = glGetUniformLocation(sunShader.getId(), "MVP");

        glm::mat4 ModelMatrix = glm::mat4(1.0);
        ModelMatrix = glm::translate(ModelMatrix, lightPos);
        glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

        sun.draw(sunShader);

        //// Switch to main shader and set uniforms ////
        shader.use();
        GLuint MatrixID2 = glGetUniformLocation(shader.getId(), "MVP");
        GLuint ModelMatrixID = glGetUniformLocation(shader.getId(), "model");
        GLuint TintID = glGetUniformLocation(shader.getId(), "objectTint");

        // Set lighting uniforms once
        glUniform3f(glGetUniformLocation(shader.getId(), "lightColor"), lightColor.x, lightColor.y, lightColor.z);
        glUniform3f(glGetUniformLocation(shader.getId(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(shader.getId(), "viewPos"),
            camera.getCameraPosition().x,
            camera.getCameraPosition().y,
            camera.getCameraPosition().z);

        //// Render terrain ////
        glm::mat4 ModelMatrixTerrain = glm::mat4(1.0f);
        glm::mat4 MVPTerrain = ProjectionMatrix * ViewMatrix * ModelMatrixTerrain;

        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVPTerrain[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrixTerrain[0][0]);
        glUniform3f(TintID, 1.0f, 1.0f, 1.0f); // Normal color for terrain

        // Pass hazard zone data to shader
        const auto& hazards = gameState.getHazardZones();
        glUniform1i(glGetUniformLocation(shader.getId(), "numHazards"), hazards.size());

        for (int i = 0; i < hazards.size() && i < 10; i++) {
            std::string posName = "hazardPositions[" + std::to_string(i) + "]";
            std::string sizeName = "hazardSizes[" + std::to_string(i) + "]";

            glUniform3f(glGetUniformLocation(shader.getId(), posName.c_str()),
                hazards[i].position.x, hazards[i].position.y, hazards[i].position.z);
            glUniform3f(glGetUniformLocation(shader.getId(), sizeName.c_str()),
                hazards[i].size.x, hazards[i].size.y, hazards[i].size.z);
        }

        terrain.draw(shader);

        //// Render all objects (crates) ////
        for (const auto& obj : objects) {
            ModelMatrix = glm::mat4(1.0f);
            ModelMatrix = glm::translate(ModelMatrix, obj.position);
            ModelMatrix = glm::rotate(ModelMatrix, glm::radians(obj.rotationY), glm::vec3(0, 1, 0));
            ModelMatrix = glm::scale(ModelMatrix, obj.scale);

            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

            glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
            glUniform3f(TintID, 1.0f, 1.0f, 1.0f); // Normal color for all crates

            obj.mesh->draw(shader);
        }

        window.update();
    }

    if (gameState.isGameOver()) {
        std::cout << "\nPress any key to close the game..." << std::endl;
        std::cin.get();
    }

    return 0;
}

void processKeyboardInput()
{
    float cameraSpeed = 30.0f * deltaTime;
    float playerRadius = 2.0f;

    glm::vec3 oldPosition = camera.getCameraPosition();

    if (window.isPressed(GLFW_KEY_W)) {
        camera.keyboardMoveFront(cameraSpeed * 4);
        if (checkAllCollisions(camera.getCameraPosition(), objects, playerRadius)) {
            camera.setCameraPosition(oldPosition);
        }
    }

    if (window.isPressed(GLFW_KEY_S)) {
        camera.keyboardMoveBack(cameraSpeed * 4);
        if (checkAllCollisions(camera.getCameraPosition(), objects, playerRadius)) {
            camera.setCameraPosition(oldPosition);
        }
    }

    if (window.isPressed(GLFW_KEY_A)) {
        camera.keyboardMoveLeft(cameraSpeed);
        if (checkAllCollisions(camera.getCameraPosition(), objects, playerRadius)) {
            camera.setCameraPosition(oldPosition);
        }
    }

    if (window.isPressed(GLFW_KEY_D)) {
        camera.keyboardMoveRight(cameraSpeed);
        if (checkAllCollisions(camera.getCameraPosition(), objects, playerRadius)) {
            camera.setCameraPosition(oldPosition);
        }
    }

    if (window.isPressed(GLFW_KEY_R))
        camera.keyboardMoveUp(cameraSpeed);
    if (window.isPressed(GLFW_KEY_F))
        camera.keyboardMoveDown(cameraSpeed);

    if (window.isPressed(GLFW_KEY_LEFT))
        camera.rotateOy(cameraSpeed / 10.0f);
    if (window.isPressed(GLFW_KEY_RIGHT))
        camera.rotateOy(-cameraSpeed / 10.0f);
    if (window.isPressed(GLFW_KEY_UP))
        camera.rotateOx(cameraSpeed / 10.0f);
    if (window.isPressed(GLFW_KEY_DOWN))
        camera.rotateOx(-cameraSpeed / 10.0f);
}
