#include "Graphics/window.h"
#include "Camera/camera.h"
#include "Shaders/shader.h"
#include "Model Loading/mesh.h"
#include "Model Loading/texture.h"
#include "Model Loading/meshLoaderObj.h"
#include "GameState.h"
#include <iostream>
#include <vector>
#include <cmath>

// Function declarations
void processKeyboardInput();
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void drawHeartsHUD(int livesLeft);

// Global variables
float deltaTime = 0.0f;
float lastFrame = 0.0f;

Window window("Alien Artifact Recovery", 1920, 1080);
Camera camera(glm::vec3(0.0f, 10.0f, 780.0f));

GameState gameState;

glm::vec3 lightColor = glm::vec3(1.0f);
glm::vec3 lightPos = glm::vec3(0.0f, 40.0f, 0.0f);

// Jump / crouch parameters
float verticalVelocity = 0.0f;
bool  isGrounded = true;
bool  isCrouching = false;

const float GROUND_Y = 10.0f;
const float JUMP_SPEED = 35.0f;
const float GRAVITY = -90.0f;
const float CROUCH_OFFSET = 4.0f;
const float STAND_HEIGHT = GROUND_Y;
const float CROUCH_HEIGHT = GROUND_Y - CROUCH_OFFSET;

// Player collision shape
const float PLAYER_RADIUS = 2.0f;
const float PLAYER_HEIGHT = 6.0f;

// Mouse look parameters
float yaw = -90.0f;
float pitch = 0.0f;
float lastX = 1280.0f / 2.0f;
float lastY = 720.0f / 2.0f;
bool  firstMouse = true;

// Camera FOV (zoom)
float fov = 90.0f;

// Lives (falls allowed)
int maxLives = 3;
int lives = maxLives;

// Pit fall state
bool  isFallingInPit = false;
float fallStartTime = 0.0f;
float fallDuration = 2.0f;     // seconds for fall animation
glm::vec3 respawnPoint = glm::vec3(0.0f, STAND_HEIGHT, 780.0f);

// Structure to hold object instance data
struct ObjectInstance {
    Mesh* mesh;
    glm::vec3 position;
    glm::vec3 scale;
    float rotationY;
};

std::vector<ObjectInstance> objects;

// HUD shader + quad
Shader* hudShader = nullptr;
Mesh* hudQuad = nullptr;

// ---------- COLLISION HELPERS FOR BOXES ----------

bool checkCollision3D(const glm::vec3& playerPos,
    const glm::vec3& objectPos,
    const glm::vec3& objectScale)
{
    float playerHalfWidth = PLAYER_RADIUS;
    float playerHalfDepth = PLAYER_RADIUS;
    float playerHalfHeight = PLAYER_HEIGHT * 0.5f;

    float pMinX = playerPos.x - playerHalfWidth;
    float pMaxX = playerPos.x + playerHalfWidth;
    float pMinY = playerPos.y - playerHalfHeight;
    float pMaxY = playerPos.y + playerHalfHeight;
    float pMinZ = playerPos.z - playerHalfDepth;
    float pMaxZ = playerPos.z + playerHalfDepth;

    float oHalfX = objectScale.x;
    float oHalfY = objectScale.y;
    float oHalfZ = objectScale.z;

    float oMinX = objectPos.x - oHalfX;
    float oMaxX = objectPos.x + oHalfX;
    float oMinY = objectPos.y - oHalfY;
    float oMaxY = objectPos.y + oHalfY;
    float oMinZ = objectPos.z - oHalfZ;
    float oMaxZ = objectPos.z + oHalfZ;

    bool overlapX = (pMinX <= oMaxX) && (pMaxX >= oMinX);
    bool overlapY = (pMinY <= oMaxY) && (pMaxY >= oMinY);
    bool overlapZ = (pMinZ <= oMaxZ) && (pMaxZ >= oMinZ);

    return overlapX && overlapY && overlapZ;
}

bool checkCollisionAllowJump(const glm::vec3& playerPos,
    const glm::vec3& objectPos,
    const glm::vec3& objectScale)
{
    float playerHalfHeight = PLAYER_HEIGHT * 0.5f;
    float playerFeetY = playerPos.y - playerHalfHeight;

    float objectTopY = objectPos.y + objectScale.y;

    if (playerFeetY > objectTopY)
        return false;

    return checkCollision3D(playerPos, objectPos, objectScale);
}

bool checkAllCollisions(const glm::vec3& newPosition,
    const std::vector<ObjectInstance>& objects)
{
    for (const auto& obj : objects) {
        if (checkCollisionAllowJump(newPosition, obj.position, obj.scale)) {
            return true;
        }
    }
    return false;
}

// ---------- PIT DETECTION ----------

bool isInsidePit(const glm::vec3& playerPos, const HazardZone& pit)
{
    float halfX = pit.size.x * 0.5f;
    float halfZ = pit.size.z * 0.5f;

    float minX = pit.position.x - halfX;
    float maxX = pit.position.x + halfX;
    float minZ = pit.position.z - halfZ;
    float maxZ = pit.position.z + halfZ;

    bool insideXZ = (playerPos.x >= minX && playerPos.x <= maxX &&
        playerPos.z >= minZ && playerPos.z <= maxZ);

    return insideXZ; // falling triggers as soon as you are inside pit XZ
}

bool checkAnyPitFall(const glm::vec3& playerPos, const std::vector<HazardZone>& pits)
{
    for (const auto& pit : pits) {
        if (isInsidePit(playerPos, pit))
            return true;
    }
    return false;
}

// ---------- MOUSE + SCROLL CALLBACKS ----------

void mouse_callback(GLFWwindow* glfwWin, double xpos, double ypos)
{
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);

    const float sensitivity = 0.05f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    if (yaw > 360.0f)  yaw -= 360.0f;
    if (yaw < -360.0f) yaw += 360.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction = glm::normalize(direction);

    camera.setCameraViewDirection(direction);
}

void scroll_callback(GLFWwindow* windowPtr, double xoffset, double yoffset)
{
    fov -= static_cast<float>(yoffset) * 2.0f;
    if (fov < 30.0f)  fov = 30.0f;
    if (fov > 120.0f) fov = 120.0f;
}

// ---------- TERRAIN GENERATION ----------

Mesh createTerrainMesh(float size, int divisions, GLuint textureID, const std::vector<HazardZone>& pits)
{
    std::vector<Vertex> vertices;
    std::vector<int>    indices;
    std::vector<Texture> texVec;

    for (int z = 0; z <= divisions; ++z) {
        for (int x = 0; x <= divisions; ++x) {
            float fx = -size + 2.0f * size * (float)x / (float)divisions;
            float fz = -size + 2.0f * size * (float)z / (float)divisions;

            Vertex v;

            float height = 0.5f * sin(fx * 0.05f) * cos(fz * 0.05f)
                + 0.3f * sin(fx * 0.15f) * cos(fz * 0.15f)
                + 0.2f * sin(fx * 0.3f) * cos(fz * 0.3f);

            for (const auto& pit : pits) {
                float dx = fx - pit.position.x;
                float dz = fz - pit.position.z;
                float distance = sqrt(dx * dx + dz * dz);
                float pitRadius = pit.size.x / 2.0f;

                if (distance < pitRadius) {
                    float normalizedDist = distance / pitRadius;
                    float falloff = 1.0f - normalizedDist;
                    falloff = falloff * falloff;
                    float pitDepth = 4.0f;
                    height -= pitDepth * falloff;
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

    for (int z = 0; z <= divisions; ++z) {
        for (int x = 0; x <= divisions; ++x) {
            int idx = z * (divisions + 1) + x;

            glm::vec3 pos = vertices[idx].pos;

            glm::vec3 posLeft = (x > 0) ? vertices[idx - 1].pos : pos;
            glm::vec3 posRight = (x < divisions) ? vertices[idx + 1].pos : pos;
            glm::vec3 posDown = (z > 0) ? vertices[idx - (divisions + 1)].pos : pos;
            glm::vec3 posUp = (z < divisions) ? vertices[idx + (divisions + 1)].pos : pos;

            glm::vec3 tangentX = glm::normalize(posRight - posLeft);
            glm::vec3 tangentZ = glm::normalize(posUp - posDown);

            glm::vec3 normal = glm::normalize(glm::cross(tangentZ, tangentX));
            vertices[idx].normals = normal;
        }
    }

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

// ---------- HEART HUD RENDERING ----------
void drawHeartsHUD(int livesLeft)
{
    if (!hudShader || !hudQuad) return;

    glDisable(GL_DEPTH_TEST);

    hudShader->use();

    glm::mat4 ortho = glm::ortho(
        0.0f, static_cast<float>(window.getWidth()),
        0.0f, static_cast<float>(window.getHeight())
    );

    GLuint mvpLoc = glGetUniformLocation(hudShader->getId(), "MVP");
    GLuint colorLoc = glGetUniformLocation(hudShader->getId(), "color");

    float heartSize = 80.0f;
    float padding = 20.0f;
    float startX = static_cast<float>(window.getWidth()) - padding - heartSize;
    float y = static_cast<float>(window.getHeight()) - padding - heartSize;

    for (int i = 0; i < maxLives; ++i) {
        float x = startX - i * (heartSize + 15.0f);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, y, 0.0f));
        model = glm::scale(model, glm::vec3(heartSize, heartSize, 1.0f));

        glm::mat4 mvp = ortho * model;

        glm::vec3 color = (i < livesLeft)
            ? glm::vec3(1.0f, 0.0f, 0.0f)
            : glm::vec3(0.3f, 0.0f, 0.0f);

        glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvp[0][0]);
        glUniform3f(colorLoc, color.r, color.g, color.b);

        hudQuad->draw(*hudShader);
    }

    glEnable(GL_DEPTH_TEST);
}


// ---------- MAIN ----------
int main()
{
    std::cout << "=== Game start ===" << std::endl;

    glClearColor(0.4f, 0.6f, 0.8f, 1.0f);

    glfwSetInputMode(window.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window.getWindow(), mouse_callback);
    glfwSetScrollCallback(window.getWindow(), scroll_callback);

    std::cout << "Creating main shaders..." << std::endl;
    Shader shader("Shaders/vertex_shader.glsl", "Shaders/fragment_shader.glsl");
    Shader sunShader("Shaders/sun_vertex_shader.glsl", "Shaders/sun_fragment_shader.glsl");

    std::cout << "Creating HUD shader..." << std::endl;
    hudShader = new Shader("Shaders/hud_vertex_shader.glsl", "Shaders/hud_fragment_shader.glsl");
    std::cout << "HUD shader program ID: " << hudShader->getId() << std::endl;

    // Create HUD quad
    {
        std::cout << "Creating HUD quad mesh..." << std::endl;
        std::vector<Vertex> hudVertices(4);
        std::vector<int> hudIndices = { 0, 1, 2, 2, 3, 0 };

        hudVertices[0].pos = glm::vec3(0.0f, 0.0f, 0.0f);
        hudVertices[1].pos = glm::vec3(1.0f, 0.0f, 0.0f);
        hudVertices[2].pos = glm::vec3(1.0f, 1.0f, 0.0f);
        hudVertices[3].pos = glm::vec3(0.0f, 1.0f, 0.0f);

        hudVertices[0].textureCoords = glm::vec2(0.0f, 0.0f);
        hudVertices[1].textureCoords = glm::vec2(1.0f, 0.0f);
        hudVertices[2].textureCoords = glm::vec2(1.0f, 1.0f);
        hudVertices[3].textureCoords = glm::vec2(0.0f, 1.0f);

        for (int i = 0; i < 4; ++i)
            hudVertices[i].normals = glm::vec3(0.0f, 0.0f, 1.0f);

        std::vector<Texture> emptyTextures;
        hudQuad = new Mesh(hudVertices, hudIndices, emptyTextures);
        if (hudQuad)
            std::cout << "HUD quad created successfully." << std::endl;
        else
            std::cout << "ERROR: hudQuad is null after creation!" << std::endl;
    }

    GLuint sandTex = loadBMP("Resources/Textures/sand.bmp");
    GLuint crate1x1Tex = loadBMP("Resources/Models/StaticObjects/crates/Crate_1x1_Mat_BaseColor.bmp");
    GLuint crate1x1TallTex = loadBMP("Resources/Models/StaticObjects/crates/Crate_1x1_Tall_Mat_BaseColor.bmp");
    GLuint crate1x2Tex = loadBMP("Resources/Models/StaticObjects/crates/Crate_1x2_Mat_BaseColor.bmp");
    GLuint crate1x2TallTex = loadBMP("Resources/Models/StaticObjects/crates/Crate_1x2_Tall_Mat_BaseColor.bmp");
    GLuint crate2x2TallTex = loadBMP("Resources/Models/StaticObjects/crates/Crate_2x2_Tall_Mat_BaseColor.bmp");

    std::vector<Texture> crate1x1TexVec(1);
    crate1x1TexVec[0].id = crate1x1Tex;
    crate1x1TexVec[0].type = "texture_diffuse";

    std::vector<Texture> crate1x1TallTexVec(1);
    crate1x1TallTexVec[0].id = crate1x1TallTex;
    crate1x1TallTexVec[0].type = "texture_diffuse";

    std::vector<Texture> crate1x2TexVec(1);
    crate1x2TexVec[0].id = crate1x2Tex;
    crate1x2TexVec[0].type = "texture_diffuse";

    std::vector<Texture> crate1x2TallTexVec(1);
    crate1x2TallTexVec[0].id = crate1x2TallTex;
    crate1x2TallTexVec[0].type = "texture_diffuse";

    std::vector<Texture> crate2x2TallTexVec(1);
    crate2x2TallTexVec[0].id = crate2x2TallTex;
    crate2x2TallTexVec[0].type = "texture_diffuse";

    glEnable(GL_DEPTH_TEST);

    MeshLoaderObj loader;
    Mesh sun = loader.loadObj("Resources/Models/sphere.obj");

    std::vector<Mesh> staticMeshes;

    std::cout << "Loading static objects..." << std::endl;

    staticMeshes.push_back(loader.loadObj("Resources/Models/StaticObjects/crates/Crate_1x1.obj", crate1x1TexVec));
    staticMeshes.push_back(loader.loadObj("Resources/Models/StaticObjects/crates/Crate_1x1_Tall.obj", crate1x1TallTexVec));
    staticMeshes.push_back(loader.loadObj("Resources/Models/StaticObjects/crates/Crate_1x2.obj", crate1x2TexVec));
    staticMeshes.push_back(loader.loadObj("Resources/Models/StaticObjects/crates/Crate_1x2_Tall.obj", crate1x2TallTexVec));
    staticMeshes.push_back(loader.loadObj("Resources/Models/StaticObjects/crates/Crate_2x2_Tall.obj", crate2x2TallTexVec));

    std::cout << "Loaded " << staticMeshes.size() << " static object types." << std::endl;

    objects.push_back({ &staticMeshes[0], glm::vec3(-80, 0, -80),   glm::vec3(7.5, 7.5, 7.5), 0 });
    objects.push_back({ &staticMeshes[1], glm::vec3(-100, 0, -60),  glm::vec3(7.5, 7.5, 7.5), 30 });
    objects.push_back({ &staticMeshes[2], glm::vec3(-120, 0, -70),  glm::vec3(7.5, 7.5, 7.5), 60 });
    objects.push_back({ &staticMeshes[3], glm::vec3(150, 0, 120),   glm::vec3(9.0, 9.0, 9.0), 90 });
    objects.push_back({ &staticMeshes[4], glm::vec3(170, 0, 140),   glm::vec3(10.0, 10.0, 10.0), 120 });
    objects.push_back({ &staticMeshes[0], glm::vec3(-200, 0, 100),  glm::vec3(6.0, 6.0, 6.0), 150 });
    objects.push_back({ &staticMeshes[2], glm::vec3(250, 0, -150),  glm::vec3(8.0, 8.0, 8.0), 180 });
    objects.push_back({ &staticMeshes[1], glm::vec3(50, 0, 200),    glm::vec3(7.0, 7.0, 7.0), 210 });
    objects.push_back({ &staticMeshes[3], glm::vec3(-300, 0, -50),  glm::vec3(8.5, 8.5, 8.5), 240 });
    objects.push_back({ &staticMeshes[4], glm::vec3(300, 0, -200),  glm::vec3(11.0, 11.0, 11.0), 270 });

    std::cout << "Placed " << objects.size() << " object instances in the world." << std::endl;

    std::cout << "\nCreating hazard pits..." << std::endl;

    gameState.addHazardZone(glm::vec3(0, 0, 700), glm::vec3(20, 10, 20), 1, "Test Pit (ahead)");
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

    respawnPoint = glm::vec3(0.0f, STAND_HEIGHT, 780.0f);
    camera.setCameraPosition(respawnPoint);
    isGrounded = true;
    verticalVelocity = 0.0f;
    isCrouching = false;

    std::cout << "Entering main loop..." << std::endl;

    int frameCounter = 0;

    while (!window.isPressed(GLFW_KEY_ESCAPE) &&
        glfwWindowShouldClose(window.getWindow()) == 0 &&
        lives > 0)
    {
        window.clear();

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Print every 120 frames to avoid spam
        if (frameCounter % 120 == 0) {
            glm::vec3 camPos = camera.getCameraPosition();
            std::cout << "[Frame " << frameCounter
                << "] cam=(" << camPos.x << ", " << camPos.y << ", " << camPos.z << ")"
                << " lives=" << lives
                << " isFalling=" << isFallingInPit << std::endl;
        }
        frameCounter++;

        if (!isFallingInPit) {
            processKeyboardInput();

            glm::vec3 pos = camera.getCameraPosition();

            if (window.isPressed(GLFW_KEY_SPACE) && isGrounded && !isCrouching) {
                verticalVelocity = JUMP_SPEED;
                isGrounded = false;
            }

            if (!isGrounded) {
                verticalVelocity += GRAVITY * deltaTime;
                pos.y += verticalVelocity * deltaTime;

                float currentGroundY = isCrouching ? CROUCH_HEIGHT : STAND_HEIGHT;
                if (pos.y <= currentGroundY) {
                    pos.y = currentGroundY;
                    verticalVelocity = 0.0f;
                    isGrounded = true;
                }
            }
            else {
                float currentGroundY = isCrouching ? CROUCH_HEIGHT : STAND_HEIGHT;
                pos.y = currentGroundY;
                verticalVelocity = 0.0f;
            }

            camera.setCameraPosition(pos);

            if (checkAnyPitFall(camera.getCameraPosition(), gameState.getHazardZones())) {
                isFallingInPit = true;
                fallStartTime = currentFrame;
                verticalVelocity = -40.0f;
                std::cout << "You fell into a pit! Lives left after this: " << (lives - 1) << std::endl;
            }

        }
        else {
            glm::vec3 pos = camera.getCameraPosition();
            verticalVelocity += GRAVITY * deltaTime;
            pos.y += verticalVelocity * deltaTime;
            camera.setCameraPosition(pos);

            if (currentFrame - fallStartTime >= fallDuration) {
                lives--;
                isFallingInPit = false;
                verticalVelocity = 0.0f;
                isGrounded = true;
                isCrouching = false;
                camera.setCameraPosition(respawnPoint);
                std::cout << "Respawned at start. Lives: " << lives << "/" << maxLives << std::endl;
            }
        }

        glm::mat4 ProjectionMatrix = glm::perspective(
            glm::radians(fov),
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

        sunShader.use();
        GLuint MatrixID = glGetUniformLocation(sunShader.getId(), "MVP");

        glm::mat4 ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, lightPos);
        glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

        sun.draw(sunShader);

        shader.use();
        GLuint MatrixID2 = glGetUniformLocation(shader.getId(), "MVP");
        GLuint ModelMatrixID = glGetUniformLocation(shader.getId(), "model");
        GLuint TintID = glGetUniformLocation(shader.getId(), "objectTint");

        glUniform3f(glGetUniformLocation(shader.getId(), "lightColor"), lightColor.x, lightColor.y, lightColor.z);
        glUniform3f(glGetUniformLocation(shader.getId(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(shader.getId(), "viewPos"),
            camera.getCameraPosition().x,
            camera.getCameraPosition().y,
            camera.getCameraPosition().z);

        glm::mat4 ModelMatrixTerrain = glm::mat4(1.0f);
        glm::mat4 MVPTerrain = ProjectionMatrix * ViewMatrix * ModelMatrixTerrain;

        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVPTerrain[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrixTerrain[0][0]);
        glUniform3f(TintID, 1.0f, 1.0f, 1.0f);

        const auto& hazards = gameState.getHazardZones();
        glUniform1i(glGetUniformLocation(shader.getId(), "numHazards"), static_cast<int>(hazards.size()));

        for (int i = 0; i < static_cast<int>(hazards.size()) && i < 10; ++i) {
            std::string posName = "hazardPositions[" + std::to_string(i) + "]";
            std::string sizeName = "hazardSizes[" + std::to_string(i) + "]";

            glUniform3f(glGetUniformLocation(shader.getId(), posName.c_str()),
                hazards[i].position.x, hazards[i].position.y, hazards[i].position.z);
            glUniform3f(glGetUniformLocation(shader.getId(), sizeName.c_str()),
                hazards[i].size.x, hazards[i].size.y, hazards[i].size.z);
        }

        terrain.draw(shader);

        for (const auto& obj : objects) {
            ModelMatrix = glm::mat4(1.0f);
            ModelMatrix = glm::translate(ModelMatrix, obj.position);
            ModelMatrix = glm::rotate(ModelMatrix, glm::radians(obj.rotationY), glm::vec3(0, 1, 0));
            ModelMatrix = glm::scale(ModelMatrix, obj.scale);

            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

            glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
            glUniform3f(TintID, 1.0f, 1.0f, 1.0f);

            obj.mesh->draw(shader);
        }

        // Debug: check that HUD is drawn, and uniforms exist
        if (frameCounter % 120 == 0) {
            std::cout << "Calling drawHeartsHUD, lives=" << lives << std::endl;
        }
        drawHeartsHUD(lives);


        window.update();
    }

    std::cout << "\nGame over. Final lives: " << lives << std::endl;
    std::cout << "Press any key to close the game..." << std::endl;
    std::cin.get();

    return 0;
}

// ---------- KEYBOARD INPUT ----------

void processKeyboardInput()
{
    if (isFallingInPit)
        return;

    float baseSpeed = 30.0f * deltaTime;
    float cameraSpeed = isCrouching ? baseSpeed * 0.5f : baseSpeed;

    bool crouchKey = window.isPressed(GLFW_KEY_LEFT_CONTROL);
    if (crouchKey) {
        if (!isCrouching && isGrounded) {
            isCrouching = true;
            glm::vec3 p = camera.getCameraPosition();
            p.y = CROUCH_HEIGHT;
            camera.setCameraPosition(p);
        }
    }
    else {
        if (isCrouching && isGrounded) {
            isCrouching = false;
            glm::vec3 p = camera.getCameraPosition();
            p.y = STAND_HEIGHT;
            camera.setCameraPosition(p);
        }
    }

    if (window.isPressed(GLFW_KEY_W)) {
        glm::vec3 oldPos = camera.getCameraPosition();
        camera.keyboardMoveFront(cameraSpeed * 4.0f);
        if (checkAllCollisions(camera.getCameraPosition(), objects)) {
            camera.setCameraPosition(oldPos);
        }
    }

    if (window.isPressed(GLFW_KEY_S)) {
        glm::vec3 oldPos = camera.getCameraPosition();
        camera.keyboardMoveBack(cameraSpeed * 4.0f);
        if (checkAllCollisions(camera.getCameraPosition(), objects)) {
            camera.setCameraPosition(oldPos);
        }
    }

    if (window.isPressed(GLFW_KEY_A)) {
        glm::vec3 oldPos = camera.getCameraPosition();
        camera.keyboardMoveLeft(cameraSpeed);
        if (checkAllCollisions(camera.getCameraPosition(), objects)) {
            camera.setCameraPosition(oldPos);
        }
    }

    if (window.isPressed(GLFW_KEY_D)) {
        glm::vec3 oldPos = camera.getCameraPosition();
        camera.keyboardMoveRight(cameraSpeed);
        if (checkAllCollisions(camera.getCameraPosition(), objects)) {
            camera.setCameraPosition(oldPos);
        }
    }
}
