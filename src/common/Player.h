//
// Created by amit on 5/27/25.
//

#ifndef TANKSBATTLEGAME_PLAYER_H
#define TANKSBATTLEGAME_PLAYER_H

#include <stdio.h>
#include "TankAlgorithm.h"
#include "SatelliteView.h"


class Player {
public:
    Player( [[maybe_unused]]int player_index,
            [[maybe_unused]]size_t x,
            [[maybe_unused]]size_t y,
            [[maybe_unused]]size_t max_steps,
            [[maybe_unused]]size_t num_shells ) {}

    virtual ~Player() {}
    virtual void updateTankWithBattleInfo
            (TankAlgorithm& tank, SatelliteView& satellite_view) = 0;
};

#endif //TANKSBATTLEGAME_PLAYER_H
