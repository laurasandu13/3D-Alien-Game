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




