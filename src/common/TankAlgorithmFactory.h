//
// Created by amit on 5/27/25.
//

#ifndef TANKSBATTLEGAME_TANKALGORITHMFACTORY_H
#define TANKSBATTLEGAME_TANKALGORITHMFACTORY_H

#include "TankAlgorithm.h"
#include <memory>

class TankAlgorithmFactory {
public:
    virtual ~TankAlgorithmFactory() {}
    virtual std::unique_ptr<TankAlgorithm> create(
            int player_index, int tank_index) const = 0;
};

#endif //TANKSBATTLEGAME_TANKALGORITHMFACTORY_H
