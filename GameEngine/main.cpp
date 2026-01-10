#include "Graphics/window.h"
#include "Camera/camera.h"
#include "Shaders/shader.h"
#include "Model Loading/mesh.h"
#include "Model Loading/texture.h"
#include "Model Loading/meshLoaderObj.h"
#include "GameState.h"
#include <iostream>

// Function declarations
void processKeyboardInput();

// Global variables
float deltaTime = 0.0f;
float lastFrame = 0.0f;

Window window("Alien Artifact Recovery", 1280, 720);
Camera camera(glm::vec3(0.0f, 10.0f, 780.0f));  // Start position: slightly elevated

GameState gameState;  // Our new game state manager

glm::vec3 lightColor = glm::vec3(1.0f);
glm::vec3 lightPos = glm::vec3(0.0f, 40.0f, 0.0f);  // above the box

// Track E key press (for interactions later)
bool eKeyPressed = false;
bool eKeyWasPressed = false;

// function to create terrain
Mesh createTerrainMesh(float size, int divisions, GLuint textureID) {
    std::vector<Vertex> vertices;
    std::vector<int> indices;
	std::vector<Texture> texVec;

    for (int z = 0; z <= divisions; ++z) {
        for (int x = 0; x <= divisions; ++x) {
            float fx = -size + 2.0f * size * (float)x / (float)divisions;
            float fz = -size + 2.0f * size * (float)z / (float)divisions;

            Vertex v;
            
			// height variation using sine and cosine
            float height = 0.5f * sin(fx * 0.05f) * cos(fz * 0.05f)     // large slow waves
                + 0.3f * sin(fx * 0.15f) * cos(fz * 0.15f)              // medium waves
                + 0.2f * sin(fx * 0.3f) * cos(fz * 0.3f);               // small detail


            v.pos = glm::vec3(fx, height, fz); // hills

			// recalculate normals to get proper lighting on slopes
            float hL = 0.5f * sin((fx - 1.0f) * 0.05f) * cos(fz * 0.05f)
                + 0.3f * sin((fx - 1.0f) * 0.15f) * cos(fz * 0.15f)
                + 0.2f * sin((fx - 1.0f) * 0.3f) * cos(fz * 0.3f);

            float hR = 0.5f * sin((fx + 1.0f) * 0.05f) * cos(fz * 0.05f)
                + 0.3f * sin((fx + 1.0f) * 0.15f) * cos(fz * 0.15f)
                + 0.2f * sin((fx + 1.0f) * 0.3f) * cos(fz * 0.3f);

            float hD = 0.5f * sin(fx * 0.05f) * cos((fz - 1.0f) * 0.05f)
                + 0.3f * sin(fx * 0.15f) * cos((fz - 1.0f) * 0.15f)
                + 0.2f * sin(fx * 0.3f) * cos((fz - 1.0f) * 0.3f);

            float hU = 0.5f * sin(fx * 0.05f) * cos((fz + 1.0f) * 0.05f)
                + 0.3f * sin(fx * 0.15f) * cos((fz + 1.0f) * 0.15f)
                + 0.2f * sin(fx * 0.3f) * cos((fz + 1.0f) * 0.3f);

            glm::vec3 tangentX = glm::vec3(2.0f, hR - hL, 0.0f);
            glm::vec3 tangentZ = glm::vec3(0.0f, hU - hD, 2.0f);
            v.normals = glm::normalize(glm::cross(tangentZ, tangentX));

            v.textureCoords = glm::vec2(
                (float)x / (float)divisions * 10.0f,
                (float)z / (float)divisions * 10.0f
            );
            vertices.push_back(v);
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

int main()
{
    glClearColor(0.4f, 0.6f, 0.8f, 1.0f);  // Sky blue background

    // Building and compiling shader program
    Shader shader("Shaders/vertex_shader.glsl", "Shaders/fragment_shader.glsl");
    Shader sunShader("Shaders/sun_vertex_shader.glsl", "Shaders/sun_fragment_shader.glsl");

    GLuint tex = loadBMP("Resources/Textures/wood.bmp");
    GLuint tex2 = loadBMP("Resources/Textures/rock.bmp");
    GLuint tex3 = loadBMP("Resources/Textures/orange.bmp");
	GLuint sandTex = loadBMP("Resources/Textures/sand.bmp");

    glEnable(GL_DEPTH_TEST);

    MeshLoaderObj loader;
    Mesh sun = loader.loadObj("Resources/Models/sphere.obj");

    std::vector<Texture> textures;
    textures.push_back(Texture());
    textures[0].id = tex;
    textures[0].type = "texture_diffuse";

    Mesh box = loader.loadObj("Resources/Models/cube.obj", textures);

    Mesh terrain = createTerrainMesh(100.0f, 100, sandTex);

    // Print initial game state
    std::cout << "========================================" << std::endl;
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
    std::cout << "========================================\n" << std::endl;

    // Main game loop
    while (!window.isPressed(GLFW_KEY_ESCAPE) &&
        glfwWindowShouldClose(window.getWindow()) == 0)
    {
        window.clear();

        // Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processKeyboardInput();

        // Check E key for interactions (we'll use this in later phases)
        eKeyWasPressed = eKeyPressed;
        eKeyPressed = window.isPressed(GLFW_KEY_E);

        // Detect E key press (rising edge)
        if (eKeyPressed && !eKeyWasPressed) {
            std::cout << "E key pressed! (Interaction will be added in Phase 6)" << std::endl;
        }

        // Setup matrices for rendering
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

        //// Render light source ////
        sunShader.use();
        GLuint MatrixID = glGetUniformLocation(sunShader.getId(), "MVP");

        glm::mat4 ModelMatrix = glm::mat4(1.0);
        ModelMatrix = glm::translate(ModelMatrix, lightPos);
        glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

        sun.draw(sunShader);

        //// Render test box (placeholder for future objects) ////
        shader.use();
        GLuint MatrixID2 = glGetUniformLocation(shader.getId(), "MVP");
        GLuint ModelMatrixID = glGetUniformLocation(shader.getId(), "model");

        ModelMatrix = glm::mat4(1.0);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 5.0f, 0.0f));
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
        glUniform3f(glGetUniformLocation(shader.getId(), "lightColor"), lightColor.x, lightColor.y, lightColor.z);
        glUniform3f(glGetUniformLocation(shader.getId(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(shader.getId(), "viewPos"),
            camera.getCameraPosition().x,
            camera.getCameraPosition().y,
            camera.getCameraPosition().z);

        box.draw(shader);

        //// Render terrain ////
        shader.use();

        // Terrain at y = 0
        glm::mat4 ModelMatrixTerrain = glm::mat4(1.0f);
        glm::mat4 MVPTerrain = ProjectionMatrix * ViewMatrix * ModelMatrixTerrain;

        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVPTerrain[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrixTerrain[0][0]);
        glUniform3f(glGetUniformLocation(shader.getId(), "lightColor"), lightColor.x, lightColor.y, lightColor.z);
        glUniform3f(glGetUniformLocation(shader.getId(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(shader.getId(), "viewPos"),
            camera.getCameraPosition().x,
            camera.getCameraPosition().y,
            camera.getCameraPosition().z);

        terrain.draw(shader);

        //// Render test box (placeholder) ////
        ModelMatrix = glm::mat4(1.0);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 5.0f, 0.0f));
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

        box.draw(shader);


        window.update();
    }

    return 0;
}

void processKeyboardInput()
{
    float cameraSpeed = 30.0f * deltaTime;

    // Translation (WASD + R/F)
    if (window.isPressed(GLFW_KEY_W))
        camera.keyboardMoveFront(cameraSpeed);
    if (window.isPressed(GLFW_KEY_S))
        camera.keyboardMoveBack(cameraSpeed);
    if (window.isPressed(GLFW_KEY_A))
        camera.keyboardMoveLeft(cameraSpeed);
    if (window.isPressed(GLFW_KEY_D))
        camera.keyboardMoveRight(cameraSpeed);
    if (window.isPressed(GLFW_KEY_R))
        camera.keyboardMoveUp(cameraSpeed);
    if (window.isPressed(GLFW_KEY_F))
        camera.keyboardMoveDown(cameraSpeed);

    // Rotation (Arrow keys)
    if (window.isPressed(GLFW_KEY_LEFT))
        camera.rotateOy(cameraSpeed / 10.0f);
    if (window.isPressed(GLFW_KEY_RIGHT))
        camera.rotateOy(-cameraSpeed / 10.0f);
    if (window.isPressed(GLFW_KEY_UP))
        camera.rotateOx(cameraSpeed / 10.0f);
    if (window.isPressed(GLFW_KEY_DOWN))
        camera.rotateOx(-cameraSpeed / 10.0f);
}
