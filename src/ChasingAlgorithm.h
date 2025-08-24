#ifndef CHASINGALGORITHM_H
#define CHASINGALGORITHM_H

// C++ Standard Library includes
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Project includes
#include "common/ActionRequest.h"
#include "common/SatelliteView.h"
#include "common/TankAlgorithm.h"
#include "Direction.h"
#include "GameManager.h"
#include "GameState.h"
#include "Logger.h"
#include "MyBattleInfo.h"
#include "Position.h"
#include "Tank.h"

// Forward declarations
class GameObject;

// Helper function declarations
std::optional<Direction> calculateDirection(const Position& from, const Position& to, int board_width, int board_height);
ActionRequest getShortestRotation(Direction current, Direction target);
bool isBlocked(bool isPosOccupiedByWall, bool isPosOccupiedByMine);
std::vector<Position> reconstructPath(const Position& start, const Position& goal, const std::unordered_map<Position, Position>& came_from);
bool isLineOfSightClear(const Position& start, const Position& end, int board_width, int board_height, const std::vector<std::vector<char>>& board_matrix);
bool isExactlyAligned(const Position& from, const Position& to);

class ChasingAlgorithm : public TankAlgorithm {
private:
    bool shouldAskForBattleInfo = true;
    std::vector<std::vector<char>> board_matrix;
    Direction currentCannonDirection = Direction::RIGHT; // Default to RIGHT
    bool isDirectionInitialized = false; // Track if we've initialized direction based on player ID
    int cooldown_remaining = 0; // Track cooldown independently
    static constexpr int SHOOT_COOLDOWN = 4; // Same as Tank's cooldown
    int shells_remaining = Tank::initial_shells; // Track shells independently
    int player_id = 0; // Track player_id, 0 means uninitialized
    size_t board_width = 0;
    size_t board_height = 0;

    // New member variables for shell tracking
    std::unordered_map<Position, Position> previous_shell_positions; // Maps current shell positions to their previous positions
    std::unordered_map<Position, Direction> shell_directions; // Cache of inferred shell directions

    // Helper method to find tank position from board
    Position findMyTankPosition() const {
        // Scan the board for '%' which marks our tank's position
        for (size_t x = 0; x < board_width; ++x) {
            for (size_t y = 0; y < board_height; ++y) {
                if (board_matrix[x][y] == '%') {
                    return Position(x, y);
                }
            }
        }

        LOG_ERROR("Could not find tank position on board");
        return Position(0, 0); // Default position as fallback
    }

    // Helper method to find opponent tank in line with current direction
    std::optional<Position> findOpponentInLine() const {
        Position myPos = findMyTankPosition();
        Position offset = DirectionUtils::getMovementOffset(currentCannonDirection);
        Position checkPos = myPos;
        
        // Keep checking positions in the current direction until wall/edge
        while (true) {
            // Move to next position in line
            checkPos.x = (checkPos.x + offset.x + board_width) % board_width;
            checkPos.y = (checkPos.y + offset.y + board_height) % board_height;

            // Stop if we wrapped around back to our position
            if (checkPos == myPos) break;
            
            char obj = board_matrix[checkPos.x][checkPos.y];
            
            // If we hit a wall, stop searching
            if (obj == '#') break;
            
            // If we find an opponent tank (any tank not our player_id)
            if ((obj == '1' || obj == '2') &&
                std::to_string(player_id) != std::string(1, obj)) {
                return checkPos;
            }
        }
        return std::nullopt;
    }

    // Helper method to find closest opponent tank
    std::optional<Position> findClosestOpponent() const {
        Position myPos = findMyTankPosition();
        std::optional<Position> closestPos;
        size_t minDistance = std::numeric_limits<size_t>::max();

        // Search entire board for opponent tanks
        for (size_t x = 0; x < board_width; ++x) {
            for (size_t y = 0; y < board_height; ++y) {
                char obj = board_matrix[x][y];
                
                // If we find an opponent tank (any tank not our player_id)
                if ((obj == '1' || obj == '2') &&
                    std::to_string(player_id) != std::string(1, obj)) {
                    Position tankPos(x, y);
                    
                    // Calculate wrapped distance
                    int dx = std::abs(static_cast<int>(tankPos.x) - static_cast<int>(myPos.x));
                    int dy = std::abs(static_cast<int>(tankPos.y) - static_cast<int>(myPos.y));
                    
                    // Account for board wrap-around
                    dx = std::min(dx, static_cast<int>(board_width) - dx);
                    dy = std::min(dy, static_cast<int>(board_height) - dy);
                    
                    size_t distance = dx * dx + dy * dy;
                    
                    if (distance < minDistance) {
                        minDistance = distance;
                        closestPos = tankPos;
                    }
                }
            }
        }
        return closestPos;
    }

    // Finds the shortest path using Breadth-First Search (BFS)
    // Returns the path as a vector of positions (empty if no path found)
    std::vector<Position> findShortestPathBFS(
            const Position& start,
            const Position& goal)
    {
        LOG_DEBUG("BFS: Starting pathfinding from " + start.toString() + " to " + goal.toString());

        std::queue<Position> frontier;
        std::unordered_map<Position, Position> came_from;
        std::unordered_set<Position> visited; // Use set for faster lookups

        frontier.push(start);
        visited.insert(start);

        // Define potential neighbors
        std::vector<Direction> directions = {
                Direction::UP, Direction::DOWN, Direction::LEFT, Direction::RIGHT,
                Direction::UP_LEFT, Direction::UP_RIGHT, Direction::DOWN_LEFT, Direction::DOWN_RIGHT
        };

        bool found = false;
        while (!frontier.empty()) {
            Position current = frontier.front();
            frontier.pop();

            if (current == goal) {
                LOG_DEBUG("BFS: Goal reached at " + current.toString());
                found = true;
                break;
            }

            for (const auto& dir : directions) {
                Position offset = DirectionUtils::getMovementOffset(dir, 1);
                Position next = current + offset;

                // Apply wrap-around
                next.x = (next.x + board_width) % board_width;
                next.y = (next.y + board_height) % board_height;

                // Check if visited or blocked
                char nextObject = board_matrix[next.x][next.y];
                bool isWall = (nextObject == '#');
                bool isMine = (nextObject == '@');
                if (visited.find(next) == visited.end() && !isBlocked(isWall, isMine)) {
                    visited.insert(next);
                    came_from[next] = current;
                    frontier.push(next);
//                    LOG_DEBUG("BFS: Exploring neighbor " + next.toString() + " from " + current.toString());
                }
            }
        }

        if (found) {
            std::vector<Position> path = reconstructPath(start, goal, came_from);
            if (!path.empty()) {
                std::stringstream ss;
                ss << "BFS: Path found (length " << path.size() << "): ";
                for(const auto& p : path) ss << p.toString() << " ";
                LOG_DEBUG(ss.str());
            } else {
                LOG_DEBUG("BFS: Goal reached but path reconstruction failed.");
            }
            return path;
        } else {
            LOG_DEBUG("BFS: Goal not reachable.");
            return {}; // Return empty vector if no path found
        }
    }

    // Helper method to find shell positions from board
    std::vector<Position> findShellPositions() const {
        std::vector<Position> shells;
        for (size_t x = 0; x < board_width; ++x) {
            for (size_t y = 0; y < board_height; ++y) {
                if (board_matrix[x][y] == '*') {
                    shells.emplace_back(x, y);
                }
            }
        }
        return shells;
    }

    // Helper method to infer shell directions based on previous positions
    std::vector<std::pair<Position, Direction>> inferShellDirections() {
        std::vector<std::pair<Position, Direction>> inferred_directions;
        auto current_shells = findShellPositions();
        
        // For each current shell, try to match with previous positions
        for (const auto& curr_pos : current_shells) {
            // If we have its previous position
            if (previous_shell_positions.count(curr_pos)) {
                Position prev_pos = previous_shell_positions[curr_pos];
                
                // Calculate direction using existing calculateDirection function
                auto dir_opt = calculateDirection(prev_pos, curr_pos, board_width, board_height);
                if (dir_opt) {
                    inferred_directions.emplace_back(curr_pos, *dir_opt);
                    shell_directions[curr_pos] = *dir_opt; // Cache the direction
                }
            } else if (shell_directions.count(curr_pos)) {
                // Use cached direction if available
                inferred_directions.emplace_back(curr_pos, shell_directions[curr_pos]);
            }
        }
        
        // Update previous positions for next turn
        previous_shell_positions.clear();
        for (const auto& pos : current_shells) {
            previous_shell_positions[pos] = pos;
        }
        
        return inferred_directions;
    }

    // Helper method to determine if a position is in danger from shells
    bool isPositionInDanger(const Position& pos, const std::vector<std::pair<Position, Direction>>& my_shell_directions) const {
        for (const auto& [shell_pos, direction] : my_shell_directions) {
            // Check positions ahead of the shell in its direction
            Position check_pos = shell_pos;
            for (int i = 1; i <= 4; i++) { // Look ahead up to 4 spaces (2 steps worth of movement)
                Position offset = DirectionUtils::getMovementOffset(direction);
                check_pos.x = (check_pos.x + offset.x + board_width) % board_width;
                check_pos.y = (check_pos.y + offset.y + board_height) % board_height;
                
                if (check_pos == pos) {
                    return true;
                }
                
                // Stop checking if we hit a wall
                if (board_matrix[check_pos.x][check_pos.y] == '#') {
                    break;
                }
            }
        }
        return false;
    }

    // Helper method to find safe direction to move
    std::optional<Direction> findSafeDirection(const Position& current_pos,
                                               const std::vector<std::pair<Position, Direction>>& my_shell_directions) const {
        std::vector<Direction> safe_directions;

        std::vector<Direction> directions = {
                Direction::UP, Direction::DOWN, Direction::LEFT, Direction::RIGHT,
                Direction::UP_LEFT, Direction::UP_RIGHT, Direction::DOWN_LEFT, Direction::DOWN_RIGHT
        };
        
        // Check each cardinal direction
        for (const auto& dir : directions) {
            Position offset = DirectionUtils::getMovementOffset(dir);
            Position new_pos((current_pos.x + offset.x + board_width) % board_width,
                           (current_pos.y + offset.y + board_height) % board_height);
            
            // Check if the new position is safe and not blocked by walls
            if (!isPositionInDanger(new_pos, my_shell_directions) &&
                board_matrix[new_pos.x][new_pos.y] != '#' &&
                board_matrix[new_pos.x][new_pos.y] != '@') {
                safe_directions.push_back(dir);
            }
        }
        
        if (!safe_directions.empty()) {
            // Choose the direction that maximizes distance from all shells
            Direction best_dir = safe_directions[0];
            double max_min_distance = 0;
            
            for (const auto& dir : safe_directions) {
                Position offset = DirectionUtils::getMovementOffset(dir);
                Position new_pos((current_pos.x + offset.x + board_width) % board_width,
                               (current_pos.y + offset.y + board_height) % board_height);
                
                double min_distance = std::numeric_limits<double>::max();
                for (const auto& [shell_pos, _] : my_shell_directions) {
                    int dx = std::abs(static_cast<int>(new_pos.x) - static_cast<int>(shell_pos.x));
                    int dy = std::abs(static_cast<int>(new_pos.y) - static_cast<int>(shell_pos.y));
                    dx = std::min(dx, static_cast<int>(board_width) - dx);
                    dy = std::min(dy, static_cast<int>(board_height) - dy);
                    double distance = std::sqrt(dx * dx + dy * dy);
                    min_distance = std::min(min_distance, distance);
                }
                
                if (min_distance > max_min_distance) {
                    max_min_distance = min_distance;
                    best_dir = dir;
                }
            }
            return best_dir;
        }
        return std::nullopt;
    }

    // Helper method to check if positions are exactly aligned (same x or y coordinate)
    bool isExactlyAligned(const Position& from, const Position& to) const {
        return from.x == to.x || from.y == to.y;
    }

    // Helper method to check if target is in the direction we're facing
    bool isTargetInFiringLine(const Position& from, const Position& to, Direction facing) const {
        // Get the movement offset for our current direction
        Position offset = DirectionUtils::getMovementOffset(facing);
        
        // For diagonal directions, both dx and dy must match the ratio
        if (facing == Direction::UP_RIGHT || facing == Direction::UP_LEFT ||
            facing == Direction::DOWN_RIGHT || facing == Direction::DOWN_LEFT) {
            int dx = to.x - from.x;
            int dy = to.y - from.y;
            return (dx * offset.y == dy * offset.x); // Check if slopes match
        }
        
        // For cardinal directions, check if target is exactly in that line
        if (facing == Direction::RIGHT || facing == Direction::LEFT) {
            return from.y == to.y;
        }
        if (facing == Direction::UP || facing == Direction::DOWN) {
            return from.x == to.x;
        }
        
        return false;
    }

public:
    ChasingAlgorithm() {
        // Direction will be initialized in first getAction call
        isDirectionInitialized = false;
        cooldown_remaining = 0;
        player_id = 0;
    }

    ActionRequest getAction() override {
        // Decrement cooldown at the start of each turn
        if (cooldown_remaining > 0) {
            cooldown_remaining--;
            LOG_DEBUG("Cooldown decremented to: " + std::to_string(cooldown_remaining));
        }

        if (shouldAskForBattleInfo) {
            shouldAskForBattleInfo = !shouldAskForBattleInfo;
            return ActionRequest::GetBattleInfo;
        }
        shouldAskForBattleInfo = !shouldAskForBattleInfo;

        LOG_DEBUG("ChasingAlgorithm::getAction called for player " + std::to_string(player_id));

        // Get own tank position
        Position myPosition = findMyTankPosition();
        LOG_DEBUG("My Tank (P" + std::to_string(player_id) + ") at " + myPosition.toString() +
                  " facing " + DirectionUtils::toString(currentCannonDirection) +
                  ", Cooldown: " + std::to_string(cooldown_remaining) +
                  ", Shells: " + std::to_string(shells_remaining));

        // First check for shell threats
        auto my_shell_directions = inferShellDirections();
        if (!my_shell_directions.empty()) {
            LOG_DEBUG("Found " + std::to_string(my_shell_directions.size()) + " shells with inferred directions");
        }

        // If current position is in danger, try to move to safety
        if (isPositionInDanger(myPosition, my_shell_directions)) {
            LOG_INFO("Current position in danger from shells, attempting evasive action");
            auto safe_dir_opt = findSafeDirection(myPosition, my_shell_directions);
            
            if (safe_dir_opt) {
                Direction safe_dir = *safe_dir_opt;
                LOG_INFO("Found safe direction: " + DirectionUtils::toString(safe_dir));
                
                // If we're not facing the safe direction, rotate towards it
                if (currentCannonDirection != safe_dir) {
                    ActionRequest rotation = getShortestRotation(currentCannonDirection, safe_dir);
                    if (rotation != ActionRequest::DoNothing) {
                        switch (rotation) {
                            case ActionRequest::RotateLeft45:
                                currentCannonDirection = DirectionUtils::rotate(currentCannonDirection, -1);
                                break;
                            case ActionRequest::RotateRight45:
                                currentCannonDirection = DirectionUtils::rotate(currentCannonDirection, 1);
                                break;
                            case ActionRequest::RotateLeft90:
                                currentCannonDirection = DirectionUtils::rotate(currentCannonDirection, -2);
                                break;
                            case ActionRequest::RotateRight90:
                                currentCannonDirection = DirectionUtils::rotate(currentCannonDirection, 2);
                                break;
                            default:
                                break;
                        }
                        return rotation;
                    }
                }
                
                // If we're facing the safe direction, move
                if (currentCannonDirection == safe_dir) {
                    return ActionRequest::MoveForward;
                }
            } else {
                LOG_WARNING("No safe direction found while in danger!");
            }
        }

        // If we're not in immediate danger, proceed with normal targeting behavior
        // First priority: Find opponent in line with current direction
        std::optional<Position> targetPos = findOpponentInLine();
        
        // Second priority: Find closest opponent if none in line
        if (!targetPos) {
            targetPos = findClosestOpponent();
            LOG_DEBUG("No opponent in line, searching for closest opponent");
        }

        if (!targetPos) {
            LOG_INFO("No opponent tank found. Rotating Right45 default action.");
            currentCannonDirection = DirectionUtils::rotate(currentCannonDirection, 1);
            return ActionRequest::RotateRight45;
        }

        LOG_DEBUG("Target opponent found at " + targetPos->toString());

        // Check if we can shoot the target
        bool los_clear = isLineOfSightClear(myPosition, *targetPos, board_width, board_height, board_matrix);
        std::optional<Direction> direct_direction_opt = calculateDirection(myPosition, *targetPos, board_width, board_height);

        if (direct_direction_opt) {
            Direction directDirection = *direct_direction_opt;
            LOG_DEBUG("Direct direction to opponent: " + DirectionUtils::toString(directDirection));
            LOG_DEBUG("Checking shoot condition: LoS=" + std::string(los_clear ? "Clear" : "Blocked") +
                      ", FacingDirectly=" + std::string(currentCannonDirection == directDirection ? "Yes" : "No") +
                      ", Cooldown=" + std::to_string(cooldown_remaining) +
                      ", Shells=" + std::to_string(shells_remaining));

            if (los_clear &&
                currentCannonDirection == directDirection &&
                isTargetInFiringLine(myPosition, *targetPos, currentCannonDirection) &&
                cooldown_remaining == 0 &&
                shells_remaining > 0)
            {
                LOG_INFO("ActionRequest Selected: Shoot (Direct LoS Clear)");
                cooldown_remaining = SHOOT_COOLDOWN;
                shells_remaining--;
                LOG_DEBUG("Shot fired. Shells remaining: " + std::to_string(shells_remaining));
                return ActionRequest::Shoot;
            }
        }

        // If we can't shoot, try to move towards the target
        std::vector<Position> path = findShortestPathBFS(myPosition, *targetPos);

        if (!path.empty()) {
            Position nextPosition = path[0];
            LOG_DEBUG("Path found. Next step target: " + nextPosition.toString());

            // Before moving, check if the next position would put us in danger
            if (!isPositionInDanger(nextPosition, my_shell_directions)) {
                std::optional<Direction> required_direction_opt = calculateDirection(myPosition, nextPosition, board_width, board_height);

                if (!required_direction_opt) {
                    LOG_WARNING("Could not calculate direction for next step. Rotating Right45 default action.");
                    currentCannonDirection = DirectionUtils::rotate(currentCannonDirection, 1);
                    return ActionRequest::RotateRight45;
                }

                Direction requiredDirection = *required_direction_opt;
                LOG_DEBUG("Required direction for next step: " + DirectionUtils::toString(requiredDirection));

                if (currentCannonDirection == requiredDirection) {
                    char nextObject = board_matrix[nextPosition.x][nextPosition.y];
                    bool isBlocked = (nextObject == '#' || nextObject == '@' || 
                                    (nextObject == '1' || nextObject == '2'));

                    if (!isBlocked) {
                        LOG_INFO("ActionRequest Selected: MoveForward (Following Path)");
                        return ActionRequest::MoveForward;
                    }
                }

                // Rotate towards required direction
                ActionRequest rotationAction = getShortestRotation(currentCannonDirection, requiredDirection);
                if (rotationAction != ActionRequest::DoNothing) {
                    LOG_INFO("ActionRequest Selected: " + actionToString(rotationAction) + " (Aligning for Path)");
                    switch (rotationAction) {
                        case ActionRequest::RotateLeft45:
                            currentCannonDirection = DirectionUtils::rotate(currentCannonDirection, -1);
                            break;
                        case ActionRequest::RotateRight45:
                            currentCannonDirection = DirectionUtils::rotate(currentCannonDirection, 1);
                            break;
                        case ActionRequest::RotateLeft90:
                            currentCannonDirection = DirectionUtils::rotate(currentCannonDirection, -2);
                            break;
                        case ActionRequest::RotateRight90:
                            currentCannonDirection = DirectionUtils::rotate(currentCannonDirection, 2);
                            break;
                        default:
                            break;
                    }
                    return rotationAction;
                }
            } else {
                LOG_INFO("Next position in path is dangerous, avoiding it");
            }
        }

        // Fallback: rotate to search
        LOG_INFO("No clear path to target. Rotating Right45 default action.");
        currentCannonDirection = DirectionUtils::rotate(currentCannonDirection, 1);
        return ActionRequest::RotateRight45;
    }

    void updateBattleInfo(BattleInfo& info) override {
        auto* myBattleInfo = dynamic_cast<MyBattleInfo*>(&info);
        if (!myBattleInfo) {
            LOG_ERROR("Failed to cast BattleInfo to MyBattleInfo");
            return;
        }

        // Update board dimensions
        board_width = myBattleInfo->getBoardWidth();
        board_height = myBattleInfo->getBoardHeight();

        // Create board matrix from battle info
        board_matrix = std::vector<std::vector<char>>(board_width, std::vector<char>(board_height, ' '));
        for (size_t x = 0; x < board_width; ++x) {
            for (size_t y = 0; y < board_height; ++y) {
                board_matrix[x][y] = myBattleInfo->getObjectAt(x, y);
            }
        }

        // Update player_id from battleInfo when we have it
        if (player_id == 0) {
            player_id = myBattleInfo->getPlayerId();
            LOG_DEBUG("Updated player_id to: " + std::to_string(player_id));
        }

        // Initialize direction based on player ID on first call
        if (!isDirectionInitialized) {
            currentCannonDirection = player_id == 1 ? Direction::LEFT : Direction::RIGHT;  // Player 1 starts facing LEFT, Player 2 starts facing RIGHT
            isDirectionInitialized = true;
            LOG_DEBUG("Initialized cannon direction to: " + DirectionUtils::toString(currentCannonDirection));
        }
    }
};

#endif // CHASINGALGORITHM_H