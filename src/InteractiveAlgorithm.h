#ifndef TANKSBATTLEGAME_INTERACTIVEALGORITHM_H
#define TANKSBATTLEGAME_INTERACTIVEALGORITHM_H

#include <iostream>
#include <limits>
#include "common/TankAlgorithm.h"
#include "common/ActionRequest.h"

class InteractiveAlgorithm : public TankAlgorithm {
public:
    InteractiveAlgorithm(int playerId, int tankId) 
        : playerId(playerId), tankId(tankId) {}

    ActionRequest getAction() override {
        std::cout << "Player " << playerId << ", enter your action for tank " << tankId << ":\n"
                  << "  w: MoveForward, s: MoveBackward\n"
                  << "  q: RotateLeft45, a: RotateLeft90\n"
                  << "  e: RotateRight45, d: RotateRight90\n"
                  << "  k: Shoot\n"
                  << "  i: GetBattleInfo\n"
                  << "Action: ";

        char input_char;
        while (true) {
            std::cin >> input_char;

            // Basic input validation and clearing buffer on bad input
            if (std::cin.fail()) {
                std::cin.clear(); // Clear error flags
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard bad input
                std::cout << "Invalid input type. Try again: ";
                continue;
            }
            // Clear any extra characters entered on the line
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            switch (input_char) {
                case 'w': return ActionRequest::MoveForward;
                case 's': return ActionRequest::MoveBackward;
                case 'q': return ActionRequest::RotateLeft45;
                case 'a': return ActionRequest::RotateLeft90;
                case 'e': return ActionRequest::RotateRight45;
                case 'd': return ActionRequest::RotateRight90;
                case 'k': return ActionRequest::Shoot;
                case 'i': return ActionRequest::GetBattleInfo;
                default:
                    std::cout << "Unknown command '" << input_char << "'. Try again: ";
                    break; // Continue loop
            }
        }
    }

    void updateBattleInfo(BattleInfo& info) override {
        // Nothing to do here for interactive algorithm
        (void)info; // Suppress unused parameter warning
    }

private:
    int playerId;
    int tankId;
};

#endif // TANKSBATTLEGAME_INTERACTIVEALGORITHM_H 