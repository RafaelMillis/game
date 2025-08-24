#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "Position.h"

enum class GameObjectType {
    TANK,
    SHELL,
    WALL,
    MINE,
    NONE
};

class GameObject {
public:
    Position position;
    char symbol;
    bool is_destroyed;
    GameObjectType type;

    GameObject(Position pos, char sym, GameObjectType obj_type)
        : position(pos), symbol(sym), is_destroyed(false), type(obj_type) {}
    
    virtual ~GameObject() = default;
    
    virtual char render() const {
        return is_destroyed ? ' ' : symbol;
    }
};

#endif // GAMEOBJECT_H