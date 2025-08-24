#ifndef ACTION_H
#define ACTION_H

#include <string>

// Enum for possible actions a tank can take
enum class ActionRequest {
    MoveForward,
    MoveBackward,
    RotateRight45,
    RotateLeft45,
    RotateRight90,
    RotateLeft90,
    Shoot,
    DoNothing, // Added for clarity, though not strictly used by algorithms
    GetBattleInfo
};


// Helper function to convert ActionRequest enum to string
inline std::string actionToString(ActionRequest action) {
    switch (action) {
        case ActionRequest::MoveForward:    return "MoveForward";
        case ActionRequest::MoveBackward:   return "MoveBackward";
        case ActionRequest::RotateRight45: return "RotateRight45";
        case ActionRequest::RotateLeft45:  return "RotateLeft45";
        case ActionRequest::RotateRight90: return "RotateRight90";
        case ActionRequest::RotateLeft90:  return "RotateLeft90";
        case ActionRequest::Shoot:           return "Shoot";
        case ActionRequest::DoNothing:            return "DoNothing"; // Added case
        case ActionRequest::GetBattleInfo: return "GetBattleInfo";
        default:                      return "UNKNOWN";
    }
}


#endif // ACTION_H
