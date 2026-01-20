#include "camera.h"

// Keep camera basis orthonormal
static void updateCameraBasis(glm::vec3& viewDir,
    glm::vec3& up,
    glm::vec3& right)
{
    viewDir = glm::normalize(viewDir);

    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

    right = glm::normalize(glm::cross(viewDir, worldUp));
    up = glm::normalize(glm::cross(right, viewDir));
}

Camera::Camera(glm::vec3 cameraPosition)
{
    this->cameraPosition = cameraPosition;
    this->cameraViewDirection = glm::vec3(0.0f, 0.0f, -1.0f);
    this->cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    this->cameraRight = glm::cross(cameraViewDirection, cameraUp);
    this->rotationOx = 0.0f;
    this->rotationOy = -90.0f;
    updateCameraBasis(this->cameraViewDirection, this->cameraUp, this->cameraRight);
}

Camera::Camera()
{
    cameraPosition = glm::vec3(0.0f, 0.0f, 100.0f);
    cameraViewDirection = glm::vec3(0.0f, 0.0f, -1.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    cameraRight = glm::cross(cameraViewDirection, cameraUp);
    rotationOx = 0.0f;
    rotationOy = -90.0f;
    updateCameraBasis(cameraViewDirection, cameraUp, cameraRight);
}

Camera::Camera(glm::vec3 cameraPosition,
    glm::vec3 cameraViewDirection,
    glm::vec3 cameraUp)
{
    this->cameraPosition = cameraPosition;
    this->cameraViewDirection = cameraViewDirection;
    this->cameraUp = cameraUp;
    this->cameraRight = glm::cross(cameraViewDirection, cameraUp);
    this->rotationOx = 0.0f;
    this->rotationOy = -90.0f;
    updateCameraBasis(this->cameraViewDirection, this->cameraUp, this->cameraRight);
}

Camera::~Camera() {}

void Camera::keyboardMoveFront(float cameraSpeed)
{
    glm::vec3 dir(cameraViewDirection.x, 0.0f, cameraViewDirection.z);
    float len = glm::length(dir);
    if (len > 0.0f)
        dir /= len;
    cameraPosition += dir * cameraSpeed;
}

void Camera::keyboardMoveBack(float cameraSpeed)
{
    glm::vec3 dir(cameraViewDirection.x, 0.0f, cameraViewDirection.z);
    float len = glm::length(dir);
    if (len > 0.0f)
        dir /= len;
    cameraPosition -= dir * cameraSpeed;
}

void Camera::keyboardMoveLeft(float cameraSpeed)
{
    glm::vec3 rightXZ(cameraRight.x, 0.0f, cameraRight.z);
    float len = glm::length(rightXZ);
    if (len > 0.0f)
        rightXZ /= len;
    cameraPosition -= rightXZ * cameraSpeed;
}

void Camera::keyboardMoveRight(float cameraSpeed)
{
    glm::vec3 rightXZ(cameraRight.x, 0.0f, cameraRight.z);
    float len = glm::length(rightXZ);
    if (len > 0.0f)
        rightXZ /= len;
    cameraPosition += rightXZ * cameraSpeed;
}

void Camera::keyboardMoveUp(float cameraSpeed)
{
    cameraPosition += cameraUp * cameraSpeed;
}

void Camera::keyboardMoveDown(float cameraSpeed)
{
    cameraPosition -= cameraUp * cameraSpeed;
}

void Camera::rotateOx(float angle)
{
    cameraViewDirection = glm::normalize(
        glm::vec3(glm::rotate(glm::mat4(1.0f), angle, cameraRight) *
            glm::vec4(cameraViewDirection, 1.0f))
    );

    updateCameraBasis(cameraViewDirection, cameraUp, cameraRight);
}

void Camera::rotateOy(float angle)
{
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

    cameraViewDirection = glm::normalize(
        glm::vec3(glm::rotate(glm::mat4(1.0f), angle, worldUp) *
            glm::vec4(cameraViewDirection, 1.0f))
    );

    updateCameraBasis(cameraViewDirection, cameraUp, cameraRight);
}

void Camera::setCameraPosition(glm::vec3 position)
{
    cameraPosition = position;
}

void Camera::setCameraViewDirection(glm::vec3 direction)
{
    cameraViewDirection = glm::normalize(direction);
    updateCameraBasis(cameraViewDirection, cameraUp, cameraRight);
}

glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(cameraPosition,
        cameraPosition + cameraViewDirection,
        cameraUp);
}

glm::vec3 Camera::getCameraPosition()
{
    return cameraPosition;
}

glm::vec3 Camera::getCameraViewDirection()
{
    return cameraViewDirection;
}

glm::vec3 Camera::getCameraUp()
{
    return cameraUp;
}
