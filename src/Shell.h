#ifndef SHELL_H
#define SHELL_H

#include "GameObject.h" // Base class
#include "Direction.h"  // For direction enum/utils
#include "Position.h"   // For Position class (used in GameObject)

// Represents a shell projectile fired by a tank
class Shell : public GameObject {
public:
    static constexpr int speed = 2;
    int movement_progress = 0;
    Direction direction; // Direction the shell is traveling

    /**
     * @brief Constructor for the Shell class.
     * @param pos Initial position of the shell (usually tank's position).
     * @param dir Direction the shell travels.
     */
    Shell(Position pos, Direction dir)
            : GameObject(pos, '*', GameObjectType::SHELL), // Symbol is '*'
              direction(dir) {} // Store the owner ID

    /**
     * @brief Moves the shell according to its direction and speed.
     * Shells typically move 2 units per step in this game.
     * Handles board wrapping.
     * @param board_width Width of the game board.
     * @param board_height Height of the game board.
     */
    void move(int board_width, int board_height) {
        // Calculate offset for 2 units of movement
        Position offset = DirectionUtils::getMovementOffset(direction, 2);

        // Apply movement with board wrapping
        position.x = (position.x + offset.x + board_width) % board_width;
        position.y = (position.y + offset.y + board_height) % board_height;
    }

    // Inherits render() method from GameObject
};

#endif // SHELL_H
