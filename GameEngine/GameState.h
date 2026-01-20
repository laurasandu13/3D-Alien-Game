#pragma once
#include <glm.hpp>
#include <vector>
#include <string>
#include <iostream>

// Structure representing a hazard zone
struct HazardZone {
    glm::vec3 position;
    glm::vec3 size;
    int damage;
    std::string name;
};

// Game state management class
class GameState {
private:
    // Player health
    int playerHealth;
    int maxHealth;

    // Health regeneration
    float healthRegenCooldown;
    float lastDamageTime;
    float lastRegenTime;

    // Game state
    bool gameOver;
    int currentTask;

    // Hazard zones
    std::vector<HazardZone> hazardZones;

public:
    GameState()
        : playerHealth(3)
        , maxHealth(3)
        , healthRegenCooldown(3.0f)
        , lastDamageTime(0.0f)
        , lastRegenTime(0.0f)
        , gameOver(false)
        , currentTask(0)
    {
    }

    // Health management
    int getPlayerHealth() const { return playerHealth; }
    int getMaxHealth() const { return maxHealth; }
    bool isGameOver() const { return gameOver; }

    void takeDamage(int amount, float currentTime) {
        playerHealth -= amount;
        lastDamageTime = currentTime;

        std::cout << "DAMAGE TAKEN! Health: " << playerHealth << "/" << maxHealth << std::endl;

        if (playerHealth <= 0) {
            playerHealth = 0;
            gameOver = true;
            std::cout << "\n========================================" << std::endl;
            std::cout << "  GAME OVER - You died!" << std::endl;
            std::cout << "========================================" << std::endl;
        }
    }

    void regenerateHealth(float currentTime) {
        if (playerHealth < maxHealth &&
            currentTime - lastDamageTime >= healthRegenCooldown &&
            currentTime - lastRegenTime >= 1.0f) {

            playerHealth++;
            lastRegenTime = currentTime;
            std::cout << "Health regenerated: " << playerHealth << "/" << maxHealth << std::endl;
        }
    }

    // Hazard zone management
    void addHazardZone(glm::vec3 pos, glm::vec3 size, int dmg, const std::string& name) {
        HazardZone zone;
        zone.position = pos;
        zone.size = size;
        zone.damage = dmg;
        zone.name = name;
        hazardZones.push_back(zone);
    }

    const std::vector<HazardZone>& getHazardZones() const {
        return hazardZones;
    }

    // Check if player is in any hazard zone
    bool checkHazardCollision(glm::vec3 playerPos, std::string& hazardName) {
        for (const auto& hazard : hazardZones) {
            float halfSizeX = hazard.size.x / 2.0f;
            float halfSizeZ = hazard.size.z / 2.0f;

            if (playerPos.x >= hazard.position.x - halfSizeX &&
                playerPos.x <= hazard.position.x + halfSizeX &&
                playerPos.z >= hazard.position.z - halfSizeZ &&
                playerPos.z <= hazard.position.z + halfSizeZ) {
                hazardName = hazard.name;
                return true;
            }
        }
        return false;
    }

    // Update health system (damage and regeneration)
    void updateHealthSystem(glm::vec3 playerPos, float currentTime) {
        static float lastHazardCheckTime = 0.0f;
        static bool wasInHazard = false;

        // Check for hazard damage every 0.5 seconds
        if (currentTime - lastHazardCheckTime >= 0.5f) {
            std::string hazardName;
            bool inHazard = checkHazardCollision(playerPos, hazardName);

            if (inHazard) {
                if (!wasInHazard) {
                    std::cout << "Entered hazard zone: " << hazardName << std::endl;
                }
                takeDamage(1, currentTime);
                wasInHazard = true;
            }
            else {
                if (wasInHazard) {
                    std::cout << "Exited hazard zone - safe!" << std::endl;
                }
                wasInHazard = false;
            }

            lastHazardCheckTime = currentTime;
        }

        // Try to regenerate health
        if (!wasInHazard) {
            regenerateHealth(currentTime);
        }
    }

    // Task management
    std::string getCurrentTaskDescription() const {
        switch (currentTask) {
        case 0: return "Explore the alien terrain";
        case 1: return "Collect artifacts";
        case 2: return "Activate pylons";
        case 3: return "Repair communication tower";
        case 4: return "Cross hazard zone";
        case 5: return "Reach escape ship";
        default: return "Unknown task";
        }
    }
};
