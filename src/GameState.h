#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <string>
#include "GameObject.h"
#include "Tank.h"
#include "Shell.h"
#include "Wall.h"
#include "Mine.h"

class GameState {
public:
    int board_width;
    int board_height;
    std::vector<std::shared_ptr<GameObject>> objects;

    GameState(int width, int height)
        : board_width(width), board_height(height) {}

    void addObject(std::shared_ptr<GameObject> obj) {
        objects.push_back(obj);
    }

    std::shared_ptr<Tank> getTank(int player_id) const {
        // Return the first tank found for this player (for backward compatibility)
        for (const auto& obj : objects) {
            if (obj->type == GameObjectType::TANK && !obj->is_destroyed) {
                auto tank = std::dynamic_pointer_cast<Tank>(obj);
                if (tank && tank->player_id == player_id) {
                    return tank;
                }
            }
        }
        return nullptr;
    }

    std::vector<std::shared_ptr<Tank>> getTanks(int player_id) const {
        std::vector<std::shared_ptr<Tank>> tanks;
        for (const auto& obj : objects) {
            if (obj->type == GameObjectType::TANK && !obj->is_destroyed) {
                auto tank = std::dynamic_pointer_cast<Tank>(obj);
                if (tank && tank->player_id == player_id) {
                    tanks.push_back(tank);
                }
            }
        }
        return tanks;
    }

    std::vector<std::shared_ptr<Shell>> getShells() const {
        std::vector<std::shared_ptr<Shell>> shells;
        for (const auto& obj : objects) {
            if (obj->type == GameObjectType::SHELL && !obj->is_destroyed) {
                shells.push_back(std::dynamic_pointer_cast<Shell>(obj));
            }
        }
        return shells;
    }

    bool isValidPosition(const Position& pos) const {
        return pos.x >= 0 && pos.y >= 0 && pos.x < board_width && pos.y < board_height;
    }

    std::shared_ptr<GameObject> getObjectAt(const Position& pos) const {
        for (const auto& obj : objects) {
            if (!obj->is_destroyed && obj->position == pos) {
                return obj;
            }
        }
        return nullptr;
    }

    bool isPositionOccupiedBy(const Position& pos, GameObjectType type) const {
        auto obj = getObjectAt(pos);
        return obj && obj->type == type;
    }

    std::shared_ptr<Wall> getWallAt(const Position& pos) const {
        auto obj = getObjectAt(pos);
        if (obj && obj->type == GameObjectType::WALL) {
            return std::dynamic_pointer_cast<Wall>(obj);
        }
        return nullptr;
    }

    std::shared_ptr<Mine> getMineAt(const Position& pos) const {
        auto obj = getObjectAt(pos);
        if (obj && obj->type == GameObjectType::MINE) {
            return std::dynamic_pointer_cast<Mine>(obj);
        }
        return nullptr;
    }

    // Create a copy of the current state for the algorithms to use
    std::shared_ptr<GameState> clone() const {
        auto newState = std::make_shared<GameState>(board_width, board_height);

        for (const auto& obj : objects) {
            if (obj->is_destroyed) continue;

            std::shared_ptr<GameObject> newObj;

            switch (obj->type) {
                case GameObjectType::TANK: {
                    auto tank = std::dynamic_pointer_cast<Tank>(obj);
                    auto newTank = std::make_shared<Tank>(tank->position, tank->player_id, tank->cannonDirection);
                    newTank->shells_remaining = tank->shells_remaining;
                    newTank->cooldown_remaining = tank->cooldown_remaining;
                    newObj = newTank;
                    break;
                }
                case GameObjectType::SHELL: {
                    auto shell = std::dynamic_pointer_cast<Shell>(obj);
                    newObj = std::make_shared<Shell>(shell->position, shell->direction);
                    break;
                }
                case GameObjectType::WALL: {
                    auto wall = std::dynamic_pointer_cast<Wall>(obj);
                    newObj = std::make_shared<Wall>(wall->position);
                    break;
                }
                case GameObjectType::MINE: {
                    auto mine = std::dynamic_pointer_cast<Mine>(obj);
                    newObj = std::make_shared<Mine>(mine->position);
                    break;
                }
                default:
                    continue;
            }

            if (newObj) {
                newState->addObject(newObj);
            }
        }

        return newState;
    }
};

#endif // GAMESTATE_H