#ifndef ROTATINGALGORITHM_H
#define ROTATINGALGORITHM_H

#include "common/TankAlgorithm.h" // Base class provides actionToString now
#include "Logger.h"     // For logging macros
#include "Direction.h"  // Ensure Direction enum and potentially DirectionUtils are included
#include "common/ActionRequest.h"     // Ensure ActionRequest enum is included
#include "GameState.h"  // Include GameState
#include "Tank.h"       // Include Tank definition
#include "Wall.h"       // Include Wall definition
#include "Shell.h"      // Include Shell definition for isShellChasingTank
#include "Position.h"   // Ensure Position definition is included
#include "common/SatelliteView.h"
#include "MyBattleInfo.h"
#include <random>
#include <ctime>
#include <vector>       // For storing possible actions
#include <cmath>        // For std::abs, std::sqrt
#include <algorithm>    // For std::max
#include <memory>       // For std::shared_ptr
#include <optional>     // For std::optional
#include <string>       // For std::string
#include <limits>       // For std::numeric_limits

// A simple tank algorithm with basic logic
class RotatingAlgorithm : public TankAlgorithm {
public: // Public interface
    RotatingAlgorithm() {
    }

    /**
     * @brief Determines the next action for the tank based on simple rules.
     */
    ActionRequest getAction() override {
        return ActionRequest::RotateRight45;
    }

    void updateBattleInfo([[maybe_unused]] BattleInfo& info) override {
        // Empty implementation
    }

};

#endif // ROTATINGALGORITHM_H