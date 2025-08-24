//
// Created by amit on 5/27/25.
//

#ifndef TANKSBATTLEGAME_PLAYERFACTORY_H
#define TANKSBATTLEGAME_PLAYERFACTORY_H

#include <memory>
#include "Player.h"

class PlayerFactory {
public:
    virtual ~PlayerFactory() {}
    virtual std::unique_ptr<Player> create(int player_index, size_t x, size_t y,
                                      size_t max_steps, size_t num_shells ) const = 0;
};

#endif //TANKSBATTLEGAME_PLAYERFACTORY_H
