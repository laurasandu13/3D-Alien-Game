#pragma once

#include <glm.hpp>
#include <gtx\transform.hpp>
#include <gtc\matrix_transform.hpp>
#include <gtc\type_ptr.hpp>
#include "..\Graphics\window.h"


class Camera
{
public:
    Camera(glm::vec3 cameraPosition);
    Camera();
    Camera(glm::vec3 cameraPosition,
        glm::vec3 cameraViewDirection,
        glm::vec3 cameraUp);
    ~Camera();

    void keyboardMoveFront(float cameraSpeed);
    void keyboardMoveBack(float cameraSpeed);
    void keyboardMoveLeft(float cameraSpeed);
    void keyboardMoveRight(float cameraSpeed);
    void keyboardMoveUp(float cameraSpeed);
    void keyboardMoveDown(float cameraSpeed);

    void rotateOx(float angle);
    void rotateOy(float angle);

    void setCameraPosition(glm::vec3 position);
    void setCameraViewDirection(glm::vec3 direction);

    glm::mat4 getViewMatrix();
    glm::vec3 getCameraPosition();
    glm::vec3 getCameraViewDirection();
    glm::vec3 getCameraUp();

private:
    glm::vec3 cameraPosition;
    glm::vec3 cameraViewDirection;
    glm::vec3 cameraUp;
    glm::vec3 cameraRight;

    float rotationOx;
    float rotationOy;
};

