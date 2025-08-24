//
// Created by amit on 6/5/25.
//

#ifndef TANKSBATTLEGAME_ACTIONOUTCOME_H
#define TANKSBATTLEGAME_ACTIONOUTCOME_H

#include <string>

// Enum representing the outcome of attempting a tank action/state transition
enum class ActionOutcome {
    NONE,             // No significant state change or action occurred
    SHOT_INITIATED,   // Tank is in a state to shoot, GameManager should call shoot()
    MOVE_PENDING,     // Tank intends to move (forward or backward) this step
    ROTATED,          // Tank successfully rotated
    STATE_CHANGED,    // Internal state changed (e.g., entered BWD_1) but no immediate move/shot
    INVALID_ACTION,   // The requested action was invalid in the current state (e.g., shooting on cooldown)
    RETURNING_BATTLE_INFO // Tank is ready to receive battle info
};

// Helper function to convert ActionOutcome enum to string (optional, for logging/debugging)
inline std::string outcomeToString(ActionOutcome outcome) {
    switch (outcome) {
        case ActionOutcome::NONE:             return "DoNothing";
        case ActionOutcome::SHOT_INITIATED:   return "SHOT_INITIATED";
        case ActionOutcome::MOVE_PENDING:     return "MOVE_PENDING";
        case ActionOutcome::ROTATED:          return "ROTATED";
        case ActionOutcome::STATE_CHANGED:    return "STATE_CHANGED";
        case ActionOutcome::INVALID_ACTION:   return "INVALID_ACTION";
        case ActionOutcome::RETURNING_BATTLE_INFO: return "RETURNING_BATTLE_INFO";
        default:                              return "UNKNOWN_OUTCOME";
    }
}

#endif //TANKSBATTLEGAME_ACTIONOUTCOME_H
