#ifndef TANK_H
#define TANK_H

#include <memory>
#include <string>
#include "Logger.h"
#include "GameObject.h"
#include "Direction.h"
#include "Shell.h"
#include "common/ActionRequest.h"
#include "ActionOutcome.h"

// Enum representing the tank's internal movement state
enum class TankMovementState {
    INITIAL,         // Ready for any action
    BWD_1,           // First step of backward move requested
    BWD_2,           // Second step of backward move requested
    MOVING_BWD,      // Currently executing backward movement steps
};

// Represents a tank in the game
class Tank : public GameObject {
public:
    static int next_tank_id; // Static counter for tank IDs
    TankMovementState movementState = TankMovementState::INITIAL;
    int player_id;              // ID of the player controlling the tank (1 or 2)
    int tank_id;                // Unique ID for each tank instance
    static constexpr int speed = 1; // Movement units per full step completion
    static int initial_shells; // Initial number of shells for each tank (configurable)
    int movement_progress = 0;  // Progress towards completing a move within a step
    Direction cannonDirection;  // Current direction the tank's cannon is facing
    int shells_remaining;       // Number of shells the tank has left
    int cooldown_remaining;     // Steps remaining before the tank can shoot again
    int multiplier;             // Direction multiplier for movement (1 for fwd, -1 for bwd)

    // Tracks if a move (forward or backward) is intended for the current step,
    // set by transitionMovementState and used by updateMovementProgress.
    bool move_intent_this_step = false;

    /**
     * @brief Constructor for the Tank class.
     * @param pos Initial position of the tank.
     * @param id Player ID (1 or 2).
     * @param initialDirection Initial direction the cannon faces.
     */
    Tank(Position pos, int id, Direction initialDirection)
            : GameObject(pos, id == 1 ? '1' : '2', GameObjectType::TANK), // Use '1' or '2' as symbol
              player_id(id),
              tank_id(next_tank_id++),
              cannonDirection(initialDirection),
              shells_remaining(initial_shells), // Use the static constant
              cooldown_remaining(0) {} // Starts with no cooldown

    /**
     * @brief Get the unique tank ID.
     * @return The tank's unique ID.
     */
    int getTankId() const {
        return tank_id;
    }

    /**
     * @brief Rotates the tank's cannon by a given number of 45-degree steps.
     * @param steps Number of steps to rotate (positive for clockwise, negative for counter-clockwise).
     */
    void rotate(int steps) {
        cannonDirection = DirectionUtils::rotate(cannonDirection, steps);
        // Note: Rotation is considered instantaneous within the state transition
    }

    /**
     * @brief Transitions the tank's internal movement state based on the requested action.
     * Determines if a move is initiated, a shot is possible, or a rotation occurs.
     * Does NOT directly create shells.
     * @param action The action requested by the algorithm.
     * @return ActionOutcome indicating the result of the transition attempt.
     */
    ActionOutcome transitionMovementState(ActionRequest action) {
        // Reset move intent at the start of transition check
        move_intent_this_step = false;

        // Handle GetBattleInfo action regardless of state
        if (action == ActionRequest::GetBattleInfo) {
            return ActionOutcome::RETURNING_BATTLE_INFO;
        }

        switch(movementState) {
            case TankMovementState::INITIAL: {
                switch(action) {
                    case ActionRequest::MoveForward: {
                        multiplier = 1;
                        move_intent_this_step = true; // Set intent
                        return ActionOutcome::MOVE_PENDING;
                    }
                    case ActionRequest::MoveBackward: {
                        movementState = TankMovementState::BWD_1;
                        return ActionOutcome::STATE_CHANGED; // State changed, no immediate move/shot
                    }
                    case ActionRequest::RotateLeft45: {
                        rotate(-1);
                        return ActionOutcome::ROTATED;
                    }
                    case ActionRequest::RotateLeft90: {
                        rotate(-2);
                        return ActionOutcome::ROTATED;
                    }
                    case ActionRequest::RotateRight45: {
                        rotate(1);
                        return ActionOutcome::ROTATED;
                    }
                    case ActionRequest::RotateRight90: {
                        rotate(2);
                        return ActionOutcome::ROTATED;
                    }
                    case ActionRequest::Shoot: {
                        // Check cooldown/ammo before indicating shot is possible
                        if (shells_remaining > 0 && cooldown_remaining == 0) {
                            // Don't call shoot() here, just indicate intent
                            return ActionOutcome::SHOT_INITIATED;
                        } else {
                            // Cannot shoot due to cooldown or ammo
                            return ActionOutcome::INVALID_ACTION;
                        }
                    }
                    default: return ActionOutcome::NONE; // Or INVALID_ACTION if ActionRequest::DoNothing is passed
                }
            } // End case INITIAL

            case TankMovementState::BWD_1: {
                switch(action) {
                    case ActionRequest::MoveForward: {
                        // Cancel backward move attempt
                        movementState = TankMovementState::INITIAL;
                        return ActionOutcome::STATE_CHANGED;
                    }
                        // Any other action confirms the backward move intention
                    case ActionRequest::RotateLeft45:
                    case ActionRequest::RotateLeft90:
                    case ActionRequest::RotateRight45:
                    case ActionRequest::RotateRight90:
                    case ActionRequest::Shoot: // Shoot is ignored/deferred here
                    case ActionRequest::MoveBackward: {
                        movementState = TankMovementState::BWD_2;
                        return ActionOutcome::STATE_CHANGED; // Progressing backward state
                    }
                    default: return ActionOutcome::NONE;
                }
            } // End case BWD_1

            case TankMovementState::BWD_2: {
                switch(action) {
                    case ActionRequest::MoveForward: {
                        // Cancel backward move attempt
                        movementState = TankMovementState::INITIAL;
                        return ActionOutcome::STATE_CHANGED;
                    }
                        // Any other action confirms the backward move
                    case ActionRequest::RotateLeft45:
                    case ActionRequest::RotateLeft90:
                    case ActionRequest::RotateRight45:
                    case ActionRequest::RotateRight90:
                    case ActionRequest::Shoot: // Shoot is ignored/deferred here
                    case ActionRequest::MoveBackward: {
                        movementState = TankMovementState::MOVING_BWD;
                        multiplier = -1;
                        move_intent_this_step = true; // Set intent
                        return ActionOutcome::MOVE_PENDING; // Now ready to move backward
                    }
                    default: return ActionOutcome::NONE;
                }
            } // End case BWD_2

            case TankMovementState::MOVING_BWD: {
                switch(action) {
                    case ActionRequest::MoveForward: {
                        // Switch to moving forward
                        movementState = TankMovementState::INITIAL;
                        multiplier = 1;
                        move_intent_this_step = true; // Set intent
                        return ActionOutcome::MOVE_PENDING;
                    }
                        // Can rotate or shoot immediately after completing a backward step
                    case ActionRequest::RotateLeft45: {
                        movementState = TankMovementState::INITIAL; // Exit backward state
                        rotate(-1);
                        return ActionOutcome::ROTATED;
                    }
                    case ActionRequest::RotateLeft90: {
                        movementState = TankMovementState::INITIAL;
                        rotate(-2);
                        return ActionOutcome::ROTATED;
                    }
                    case ActionRequest::RotateRight45: {
                        movementState = TankMovementState::INITIAL;
                        rotate(1);
                        return ActionOutcome::ROTATED;
                    }
                    case ActionRequest::RotateRight90: {
                        movementState = TankMovementState::INITIAL;
                        rotate(2);
                        return ActionOutcome::ROTATED;
                    }
                    case ActionRequest::Shoot: {
                        movementState = TankMovementState::INITIAL; // Exit backward state
                        // Check cooldown/ammo before indicating shot is possible
                        if (shells_remaining > 0 && cooldown_remaining == 0) {
                            // Don't call shoot() here, just indicate intent
                            return ActionOutcome::SHOT_INITIATED;
                        } else {
                            // Cannot shoot due to cooldown or ammo
                            return ActionOutcome::INVALID_ACTION;
                        }
                    }
                    case ActionRequest::MoveBackward: {
                        // Continue moving backward
                        multiplier = -1;
                        move_intent_this_step = true; // Set intent
                        return ActionOutcome::MOVE_PENDING;
                    }
                    default: return ActionOutcome::NONE;
                }
            } // End case MOVING_BWD
        } // End switch(movementState)

        return ActionOutcome::NONE; // Should not be reached
    }


    /**
     * @brief Updates the tank's movement progress for the current sub-step.
     * If enough progress is made and a move was intended, calculates the next position.
     * @param max_speed The maximum speed of any object in the game (determines sub-step granularity).
     * @param board_width Board width for wrapping.
     * @param board_height Board height for wrapping.
     * @return The intended position after this sub-step's potential movement,
     * or the current position if no move completed this sub-step.
     */
    Position updateMovementProgress(int max_speed, int board_width, int board_height) {
        // Only accumulate progress if a move was intended for this step
        if (!move_intent_this_step) {
            movement_progress = 0; // Reset progress if no move intended
            return position; // No movement calculation needed
        }

        movement_progress += speed; // Accumulate progress based on tank speed

        if (movement_progress >= max_speed) { // Check if enough progress for one grid step
            Position offset = DirectionUtils::getMovementOffset(cannonDirection, multiplier);

            // Calculate intended position with wrapping
            int intendedPositionX = (position.x + offset.x + board_width) % board_width;
            int intendedPositionY = (position.y + offset.y + board_height) % board_height;

            movement_progress -= max_speed; // Consume progress for the completed step

            // Reset move intent after completing the step for this frame?
            // Or keep it true until the next call to transitionMovementState?
            // Let's assume it stays true until the next transition call.
            // move_intent_this_step = false;

            return Position(intendedPositionX, intendedPositionY);
        }
        // Not enough progress for a full step yet in this sub-step
        return position;
    }

    /**
     * @brief Creates a shell object if the tank has ammo and is not on cooldown.
     * This function is now typically called by GameManager after transitionMovementState
     * returns SHOT_INITIATED.
     * @return A shared_ptr to the new Shell if shot, nullptr otherwise.
     */
    std::shared_ptr<Shell> shoot() {
        // Double-check conditions (though GameManager should have checked via outcome)
        if (shells_remaining > 0 && cooldown_remaining == 0) {
            shells_remaining--;
            cooldown_remaining = 4; // Set cooldown period (e.g., 4 steps)

            // --- MODIFIED LINE ---
            // Shell starts at the tank's current position
            Position shellPos = this->position;
            // --- END MODIFICATION ---

            LOG_DEBUG("Tank " + std::to_string(player_id) + " shooting from " + shellPos.toString() + ". Shells left: " + std::to_string(shells_remaining));
            return std::make_shared<Shell>(shellPos, cannonDirection);
        }
        LOG_WARNING("Tank " + std::to_string(player_id) + " attempted shoot call but failed (ammo=" + std::to_string(shells_remaining) + ", cooldown=" + std::to_string(cooldown_remaining) + ")");
        return nullptr; // Cannot shoot
    }

    /**
     * @brief Decrements the shooting cooldown timer by one step.
     */
    void decrementCooldown() {
        if (cooldown_remaining > 0) {
            cooldown_remaining--;
        }
    }
};

#endif // TANK_H
