#ifndef DIRECTION_H
#define DIRECTION_H

#include "Position.h"
#include <string>
#include <array>
#include <stdexcept> // For potential error handling

// Enum representing the 8 possible directions
enum class Direction {
    UP,         // 0
    UP_RIGHT,   // 1
    RIGHT,      // 2
    DOWN_RIGHT, // 3
    DOWN,       // 4
    DOWN_LEFT,  // 5
    LEFT,       // 6
    UP_LEFT     // 7
};

// Array of string names for each direction, matching the enum order
const std::array<std::string, 8> DIRECTION_NAMES = {
        "UP", "UP_RIGHT", "RIGHT", "DOWN_RIGHT", "DOWN", "DOWN_LEFT", "LEFT", "UP_LEFT"
};

// Utility class for direction-related operations
class DirectionUtils {
public:
    /**
     * @brief Gets the (x, y) offset for moving one step in a given direction.
     */
    static Position getMovementOffset(Direction dir, int multiplier = 1) {
        switch (dir) {
            case Direction::UP:         return Position(0, -multiplier);
            case Direction::UP_RIGHT:   return Position(multiplier, -multiplier);
            case Direction::RIGHT:      return Position(multiplier, 0);
            case Direction::DOWN_RIGHT: return Position(multiplier, multiplier);
            case Direction::DOWN:       return Position(0, multiplier);
            case Direction::DOWN_LEFT:  return Position(-multiplier, multiplier);
            case Direction::LEFT:       return Position(-multiplier, 0);
            case Direction::UP_LEFT:    return Position(-multiplier, -multiplier);
            default:                    return Position(0, 0); // Fallback
        }
    }

    /**
     * @brief Rotates a direction by a given number of 45-degree steps.
     * Positive steps are clockwise, negative steps are counter-clockwise.
     */
    static Direction rotate(Direction dir, int steps) {
        const int num_directions = 8;
        int dir_index = static_cast<int>(dir);

        // Ensure dir_index is valid (optional safety check)
        if (dir_index < 0 || dir_index >= num_directions) {
            // Consider logging an error or throwing an exception
            // LOG_ERROR("Invalid direction index in rotate: " + std::to_string(dir_index));
            return dir; // Return original direction on invalid input
        }

        // Calculate the target index correctly handling negative steps and wrapping
        // Formula: ((a % n) + n) % n ensures result is always in [0, n-1]
        // where 'a' is (dir_index + steps) and 'n' is num_directions.
        int final_index = ( (dir_index + steps) % num_directions + num_directions ) % num_directions;

        return static_cast<Direction>(final_index);
    }

    /**
 * @brief Returns the opposite direction (180 degrees rotation).
 * @param dir The direction for which to find the opposite.
 * @return Direction The opposite direction.
 */
    static Direction opposite(Direction dir) {
        // The opposite direction is always 4 steps (4 * 45 = 180 degrees) away.
        return rotate(dir, 4);
    }


    /**
     * @brief Converts a Direction enum value to its string representation.
     */
    static std::string toString(Direction dir) {
        int index = static_cast<int>(dir);
        if (index >= 0 && index < static_cast<int>(DIRECTION_NAMES.size())) {
            return DIRECTION_NAMES[index];
        }
        return "UNKNOWN"; // Handle invalid enum values gracefully
    }
};

#endif // DIRECTION_H
