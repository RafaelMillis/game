#ifndef TANKSBATTLEGAME_MYPLAYER_H
#define TANKSBATTLEGAME_MYPLAYER_H

#include "common/Player.h"
#include "common/SatelliteView.h"
#include "common/TankAlgorithm.h"
#include "ChasingAlgorithm.h"
#include "MyBattleInfo.h"
#include "MySatelliteView.h"
#include <memory>
#include <stddef.h>
#include <vector>

class MyPlayer : public Player {
private:
    int player_index;
    size_t x;
    size_t y;
    size_t max_steps;
    size_t num_shells;

public:
    MyPlayer(int player_index,
             size_t x,
             size_t y,
             size_t max_steps,
             size_t num_shells)
        : Player(player_index, x, y, max_steps, num_shells)
        , player_index(player_index)
        , x(x)
        , y(y)
        , max_steps(max_steps)
        , num_shells(num_shells) {}

    ~MyPlayer() override = default;

    void updateTankWithBattleInfo(TankAlgorithm& algorithm, SatelliteView& satelliteView) override {
        // Create board matrix from satellite view
        std::vector<std::vector<char>> board_matrix(x, std::vector<char>(y, ' '));
        for (size_t i = 0; i < x; ++i) {
            for (size_t j = 0; j < y; ++j) {
                board_matrix[i][j] = satelliteView.getObjectAt(i, j);
            }
        }
        
        // Create MySatelliteView from board matrix
        auto mySatelliteView = std::make_shared<MySatelliteView>(board_matrix);
        auto battleInfo = std::make_shared<MyBattleInfo>(mySatelliteView, x, y, player_index);
        algorithm.updateBattleInfo(*battleInfo);
    }
};

#endif //TANKSBATTLEGAME_MYPLAYER_H 