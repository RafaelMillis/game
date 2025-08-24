#ifndef WALL_H
#define WALL_H

#include "GameObject.h"

class Wall : public GameObject {
public:
    int health;

    Wall(Position pos)
        : GameObject(pos, '#', GameObjectType::WALL),
          health(2) {}

    bool takeDamage() {
        health--;
        if (health <= 0) {
            is_destroyed = true;
            return true;
        }
        return false;
    }
};

#endif // WALL_H