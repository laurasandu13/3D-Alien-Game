// manages all game logic: which tasks are completed, current health, what the player needs to do next, etc.

#pragma once
#include <string>
#include <vector>
#include <glm.hpp>

// hold information about each task

struct Task {
	int id;						// unique task number 0-4
	std::string description;	// what the player needs to do
	bool completed;				
	glm::vec3 location;         

	Task(int taskId, std::string desc, glm::vec3 loc) : 
		id(taskId), description(desc), completed(false), location(loc) {
	}
};

// interactable objects (artifacts, pylons, tower)
struct InteractableObject {
	glm::vec3 position;			
	glm::vec3 size;          // bounding box size for collision
	int linkedTaskId;		 // which task this object is linked to?
	bool activated;			 // has the player interacted with it?
	std::string objectType; 

	InteractableObject(glm::vec3 pos, glm::vec3 sz, int taskId, std::string type) :
		position(pos), size(sz), linkedTaskId(taskId), activated(false), objectType(type) {
	}
};

// NEW: hazard zones that damage the player
struct HazardZone {
	glm::vec3 position;      // center of hazard zone
	glm::vec3 size;          // dimensions (x, y, z) of the zone
	int damageAmount;        // how much damage per hit
	std::string hazardType;  // "pit", "radiation", "lava", etc.

	HazardZone(glm::vec3 pos, glm::vec3 sz, int dmg, std::string type) :
		position(pos), size(sz), damageAmount(dmg), hazardType(type) {
	}
};

// main game state manager
class GameState {
private:
	int currentTaskIndex;	 // which task is the player currently on?
	int playerHealth;
	int maxHealth;
	bool gameWon;
	bool gameOver;

	std::vector<Task> tasks; // list of all tasks
	std::vector<InteractableObject> interactables; // all objects player can interact with
	std::vector<HazardZone> hazardZones; // NEW: dangerous areas

	// NEW: damage cooldown system
	float lastDamageTime;     // time when player last took damage
	float damageCooldown;     // minimum time between damage ticks (seconds)

	// NEW: health regeneration system
	float lastHealTime;       // time when player last healed
	float healCooldown;       // time between heal ticks (seconds)
	int healAmount;           // how much health to restore per tick

public:
	GameState() {
		currentTaskIndex = 0;
		playerHealth = 3;
		maxHealth = 3;
		gameWon = false;
		gameOver = false;

		// initialize the 5 tasks
		tasks.push_back(Task(0, "Collect 3 Ancient Artifacts", glm::vec3(0, 0, 0)));
		tasks.push_back(Task(1, "Activate 3 Energy Pylons", glm::vec3(0, 0, 0)));
		tasks.push_back(Task(2, "Repair Communication Tower", glm::vec3(0, 0, 0)));
		tasks.push_back(Task(3, "Cross Hazardous Zone", glm::vec3(0, 0, 0)));
		tasks.push_back(Task(4, "Enter Spaceship and Escape", glm::vec3(0, 0, 0)));
	}

	// get the current task description to display to player
	std::string getCurrentTaskDescription() {
		if (currentTaskIndex < tasks.size()) {
			return tasks[currentTaskIndex].description;
		}
		return "All tasks completed!";
	}

	// mark current task as completed and move to next
	void completeCurrentTask() {
		if (currentTaskIndex < tasks.size()) {
			tasks[currentTaskIndex].completed = true;
			currentTaskIndex++;

			// check if all tasks are done
			if (currentTaskIndex >= tasks.size()) {
				gameWon = true;
			}
		}
	}

	// add interactable object to world
	void addInteractable(glm::vec3 pos, glm::vec3 size, int taskId, std::string type) {
		interactables.push_back(InteractableObject(pos, size, taskId, type));
	}

	// get all interactables
	std::vector<InteractableObject>& getInteractables() {
		return interactables;
	}

	// NEW: add hazard zone to world
	void addHazardZone(glm::vec3 pos, glm::vec3 size, int damage, std::string type) {
		hazardZones.push_back(HazardZone(pos, size, damage, type));
	}

	// NEW: get all hazard zones
	std::vector<HazardZone>& getHazardZones() {
		return hazardZones;
	}

	// NEW: check if player is inside any hazard zone
	bool isPlayerInHazard(glm::vec3 playerPos, HazardZone** outHazard = nullptr) {
		for (auto& hazard : hazardZones) {
			// AABB collision check
			float minX = hazard.position.x - hazard.size.x / 2.0f;
			float maxX = hazard.position.x + hazard.size.x / 2.0f;
			float minZ = hazard.position.z - hazard.size.z / 2.0f;
			float maxZ = hazard.position.z + hazard.size.z / 2.0f;

			if (playerPos.x >= minX && playerPos.x <= maxX &&
				playerPos.z >= minZ && playerPos.z <= maxZ) {
				if (outHazard != nullptr) {
					*outHazard = &hazard;
				}
				return true; // player is inside this hazard
			}
		}
		return false; // player is safe
	}

	// NEW: update health system (call every frame)
	void updateHealthSystem(glm::vec3 playerPos, float currentTime) {
		if (gameOver) return; // don't process if dead

		HazardZone* currentHazard = nullptr;
		bool inHazard = isPlayerInHazard(playerPos, &currentHazard);

		if (inHazard && currentHazard != nullptr) {
			// Player is in danger - apply damage with cooldown
			if (currentTime - lastDamageTime >= damageCooldown) {
				takeDamage(currentHazard->damageAmount);
				lastDamageTime = currentTime;
				std::cout << "[DAMAGE] You stepped into " << currentHazard->hazardType
					<< "! Health: " << playerHealth << "/" << maxHealth << std::endl;
			}
		}
		else {
			// Player is safe - regenerate health
			if (playerHealth < maxHealth && currentTime - lastHealTime >= healCooldown) {
				heal(healAmount);
				lastHealTime = currentTime;
				std::cout << "[HEAL] Health regenerated. Health: " << playerHealth << "/" << maxHealth << std::endl;
			}
		}
	}

	// damage player from hazard zones
	void takeDamage(int amount) {
		playerHealth -= amount;
		if (playerHealth <= 0) {
			playerHealth = 0;
			gameOver = true;
		}
	}

	// heal player
	void heal(int amount) {
		playerHealth += amount;
		if (playerHealth > maxHealth) {
			playerHealth = maxHealth;
		}
	}

	// reset game to start
	void resetGame() {
		currentTaskIndex = 0;
		playerHealth = maxHealth;
		gameWon = false;
		gameOver = false;

		// reset all task completion status
		for (auto& task : tasks) {
			task.completed = false;
		}

		// reset all interactables
		for (auto& obj : interactables) {
			obj.activated = false;
		}
	}

	// getters
	int getCurrentTaskIndex() { return currentTaskIndex; }
	int getPlayerHealth() { return playerHealth; }
	int getMaxHealth() { return maxHealth; }
	bool isGameWon() { return gameWon; }
	bool isGameOver() { return gameOver; }
	std::vector<Task>& getTasks() { return tasks; } 
};




