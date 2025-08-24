#ifndef MINE_H
#define MINE_H

#include "GameObject.h"

class Mine : public GameObject {
public:
    Mine(Position pos)
        : GameObject(pos, '@', GameObjectType::MINE) {}
};

#endif // MINE_H
