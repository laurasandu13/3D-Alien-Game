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
Camera camera(glm::vec3(0.0f, 5.0f, 20.0f));  // Start position: slightly elevated

GameState gameState;  // Our new game state manager

glm::vec3 lightColor = glm::vec3(1.0f);
glm::vec3 lightPos = glm::vec3(-180.0f, 100.0f, -200.0f);

// Track E key press (for interactions later)
bool eKeyPressed = false;
bool eKeyWasPressed = false;

int main()
{
    glClearColor(0.4f, 0.6f, 0.8f, 1.0f);  // Sky blue background (Fungle-inspired)

    // Building and compiling shader program
    Shader shader("Shaders/vertex_shader.glsl", "Shaders/fragment_shader.glsl");
    Shader sunShader("Shaders/sun_vertex_shader.glsl", "Shaders/sun_fragment_shader.glsl");

    // Textures (using existing ones for now)
    GLuint tex = loadBMP("Resources/Textures/wood.bmp");
    GLuint tex2 = loadBMP("Resources/Textures/rock.bmp");
    GLuint tex3 = loadBMP("Resources/Textures/orange.bmp");

    glEnable(GL_DEPTH_TEST);

    // Load test objects (we'll replace these in Phase 2-3)
    MeshLoaderObj loader;
    Mesh sun = loader.loadObj("Resources/Models/sphere.obj");

    std::vector<Texture> textures;
    textures.push_back(Texture());
    textures[0].id = tex;
    textures[0].type = "texture_diffuse";

    Mesh box = loader.loadObj("Resources/Models/cube.obj", textures);

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

        // Process input
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
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
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
